// Nets with names beginning with sys are in the system clock domain.
// Nets with names beginning with evr are in the EVR clock domain.
// Other nets ADC AXI (adcClk) clock domain.

module acquisitionHSD #(
    parameter ACQUISITION_BUFFER_CAPACITY = -1,
    parameter LONG_SEGMENT_CAPACITY       = -1,
    parameter SHORT_SEGMENT_CAPACITY      = -1,
    parameter EARLY_SEGMENTS_COUNT        = -1,
    parameter SEGMENT_PRETRIGGER_COUNT    = -1,
    parameter AXI_SAMPLES_PER_CLOCK       = -1,
    parameter AXI_SAMPLE_WIDTH            = -1,
    parameter ADC_WIDTH                   = -1,
    parameter TRIGGER_BUS_WIDTH           = -1,
    parameter DEBUG                       = "false",
   parameter ADC_RAM_CAPACITY=ACQUISITION_BUFFER_CAPACITY/AXI_SAMPLES_PER_CLOCK,
    parameter ADC_RAM_ADDRESS_WIDTH = $clog2(ADC_RAM_CAPACITY)
    ) (
    input              sysClk,
    input              sysCsrStrobe,
    input              sysTriggerConfigStrobe,
    input              sysAcqConfig1Strobe,
    input              sysAcqConfig2Strobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] sysStatus,
    output wire [31:0] sysData,
    output wire [31:0] sysProperties,
    output wire [31:0] sysTriggerLocation,
    output reg  [63:0] sysTriggerTimestamp,

    input        evrClk,
    input [63:0] evrTimestamp,

    input                                                adcClk,
    input                                                axiValid,
    input [(AXI_SAMPLES_PER_CLOCK*AXI_SAMPLE_WIDTH)-1:0] axiData,
    input                        [TRIGGER_BUS_WIDTH-1:0] eventTriggerStrobes,
    input                                                bondedWriteEnableIn,
    input                    [ADC_RAM_ADDRESS_WIDTH-1:0] bondedWriteAddressIn,
    output                                               bondedWriteEnableOut,
    output                   [ADC_RAM_ADDRESS_WIDTH-1:0] bondedWriteAddressOut
    );

// Sanity checks -- no $error in this version of Verilog
generate
if (SEGMENT_PRETRIGGER_COUNT % AXI_SAMPLES_PER_CLOCK ) begin
  SEGMENT_PRETRIGGER_COUNT_not_integer_multliple_of_AXI_SAMPLES_PER_CLOCK();
end
if (ACQUISITION_BUFFER_CAPACITY % AXI_SAMPLES_PER_CLOCK ) begin
  ACQUISITION_BUFFER_CAPACITY_not_integer_multliple_of_AXI_SAMPLES_PER_CLOCK();
end
if (ACQUISITION_BUFFER_CAPACITY % LONG_SEGMENT_CAPACITY ) begin
  ACQUISITION_BUFFER_CAPACITY_not_integer_multliple_of_LONG_SEGMENT_CAPACITY();
end
if (ACQUISITION_BUFFER_CAPACITY % SHORT_SEGMENT_CAPACITY ) begin
  ACQUISITION_BUFFER_CAPACITY_not_integer_multliple_of_SHORT_SEGMENT_CAPACITY();
end
endgenerate

localparam ADC_CLOCKS_PER_ACQUISITION = ACQUISITION_BUFFER_CAPACITY /
                                                          AXI_SAMPLES_PER_CLOCK;
localparam ACQ_COUNTER_WIDTH = $clog2(ADC_CLOCKS_PER_ACQUISITION-1);

// Number of ADC clocks consumed by trigger detection and response.
localparam TRIGGER_DETECTION_LATENCY = 4;
localparam TRIGGER_RESPONSE_LATENCY = 1;

//
// No fancy clock crossing for most signals since
// they change only when acquisition is idle.
reg acqFinish = 0, sysAcqActive = 0, sysAcqFinish_d = 0;
(* ASYNC_REG="TRUE" *) reg  sysAcqFinish_m = 0;
reg                         sysAcqFinish = 0;
reg                         sysFull = 0;
reg                         sysStartToggle = 0;
reg                   [1:0] sysSegMode = 0;
reg signed  [ADC_WIDTH-1:0] sysTriggerLevel = 0;
reg                         sysFallingEdgeTrigger = 0;
reg                         sysIsBonded = 0;
reg [TRIGGER_BUS_WIDTH-1:0] sysEventTriggerEnables = 0;
reg                  [31:0] sysAcqConfig1 = 0, sysAcqConfig2 = 0;

// Values are left-adjusted in AXI word
localparam ADC_SHIFT = AXI_SAMPLE_WIDTH - ADC_WIDTH;
localparam ADC_ALL_SAMPLES_WIDTH = AXI_SAMPLES_PER_CLOCK * ADC_WIDTH;
localparam ADC_MUX_SELECT_WIDTH = $clog2(AXI_SAMPLES_PER_CLOCK);
localparam ADC_MUX_SELECT_WIDTH_NONZERO = (ADC_MUX_SELECT_WIDTH == 0)? 1 : ADC_MUX_SELECT_WIDTH;
localparam SINGLE_SAMPLE_PER_CLOCK = AXI_SAMPLES_PER_CLOCK == 1;
wire [ACQ_COUNTER_WIDTH:0] sysPretriggerCount =
                                           sysAcqConfig1[ACQ_COUNTER_WIDTH-1:0];
wire [ACQ_COUNTER_WIDTH-1:0] sysContinuousPosttriggerCount =
    (ACQUISITION_BUFFER_CAPACITY / AXI_SAMPLES_PER_CLOCK) - sysPretriggerCount;
wire [ACQ_COUNTER_WIDTH-1:0] sysLongSegmentedPosttriggerCount =
           (LONG_SEGMENT_CAPACITY / AXI_SAMPLES_PER_CLOCK) - sysPretriggerCount;
wire [ACQ_COUNTER_WIDTH-1:0] sysShortSegmentedPosttriggerCount =
          (SHORT_SEGMENT_CAPACITY / AXI_SAMPLES_PER_CLOCK) - sysPretriggerCount;
reg [ACQ_COUNTER_WIDTH-1:0] sysPretriggerLoad;
reg [ACQ_COUNTER_WIDTH:0] sysPosttriggerLoad;

// Continuous (non-segmented) acquisition
(*mark_debug=DEBUG*) reg [ACQ_COUNTER_WIDTH:0] acqCounter;
wire acqCounterDone = acqCounter[ACQ_COUNTER_WIDTH];

// Segmented acquisition
reg [ACQ_COUNTER_WIDTH:0] sysAcqCounterSegLoad;
localparam EARLY_SEGMENTS_COUNTER_WIDTH = $clog2(EARLY_SEGMENTS_COUNT-1);
reg [EARLY_SEGMENTS_COUNTER_WIDTH:0] earlySegmentsCounter = 0;
wire earlySegmentsCounterDone =
                             earlySegmentsCounter[EARLY_SEGMENTS_COUNTER_WIDTH];

localparam LONG_SEGMENT_COUNT = ACQUISITION_BUFFER_CAPACITY /
                                                          LONG_SEGMENT_CAPACITY;
localparam SHORT_SEGMENT_COUNT = ACQUISITION_BUFFER_CAPACITY /
                                                         SHORT_SEGMENT_CAPACITY;
localparam SEGMENT_COUNTER_WIDTH = $clog2(SHORT_SEGMENT_COUNT-1);
reg [SEGMENT_COUNTER_WIDTH:0] segmentCounter = 0, sysSegmentCounterLoad = -1;
wire segmentCounterDone = segmentCounter[SEGMENT_COUNTER_WIDTH];

localparam SEG_GAP_COUNTER_WIDTH = 27;
// Note that we use SEG_GAP_COUNTER_WIDTH and not the usual
// SEG_GAP_COUNTER_WIDTH-1 on the ranges below. The extra bit
// is for the "counter done"" condition.
(*mark_debug=DEBUG*) reg [SEG_GAP_COUNTER_WIDTH:0] segGapCounter = 0;
wire segGapCounterDone = segGapCounter[SEG_GAP_COUNTER_WIDTH];
wire [SEG_GAP_COUNTER_WIDTH:0] earlySegGapCounterLoad =
          {{SEG_GAP_COUNTER_WIDTH+1-14{1'b0}}, sysAcqConfig1[31:18]};
wire [SEG_GAP_COUNTER_WIDTH:0] laterSegGapCounterLoad =
            {1'b0, sysAcqConfig2[SEG_GAP_COUNTER_WIDTH-1:0]};

///////////////////////////////////////////////////////////////////////////////
// ADC AXI Clock Domain

// Extract salient bits from AXI stream and note those above trigger threshold.
wire                             adcDataValid;
wire [ADC_ALL_SAMPLES_WIDTH-1:0] adcData;
reg                              sampleTriggerValid = 0;
reg  [AXI_SAMPLES_PER_CLOCK-1:0] sampleAboveTrigger = 0;
reg  [AXI_SAMPLES_PER_CLOCK-1:0] sampleBelowTrigger = 0;
reg                              triggerFlagsValid = 0;
reg  [AXI_SAMPLES_PER_CLOCK-1:0] triggerFlags;
reg                              triggersBeenIdle = 0;
genvar i;
generate
for (i = 0 ; i < AXI_SAMPLES_PER_CLOCK ; i = i + 1) begin
    // adcPre* are for timing improvment
    reg adcPreValid = 0;
    reg adcValid = 0;
    (*mark_debug=DEBUG*) reg signed [ADC_WIDTH-1:0] adcPre = 0;
    (*mark_debug=DEBUG*) reg signed [ADC_WIDTH-1:0] adc = 0;
    always @(posedge adcClk) begin
        adcPreValid <= axiValid;
        adcValid <= adcPreValid;
        adcPre <= axiData[i*AXI_SAMPLE_WIDTH+ADC_SHIFT+:ADC_WIDTH];
        adc <= adcPre;

        sampleTriggerValid <= adcValid;
        sampleAboveTrigger[i] <= (adc > sysTriggerLevel);
        sampleBelowTrigger[i] <= (adc < sysTriggerLevel);

        triggerFlagsValid <= sampleTriggerValid;
        triggerFlags[i] <= sysFallingEdgeTrigger ? sampleBelowTrigger[i]
                                                 : sampleAboveTrigger[i];
    end
    assign adcDataValid = adcValid;
    assign adcData[i*ADC_WIDTH+:ADC_WIDTH] = adc;
end
endgenerate

// Check for trigger condition
// Reduce chances of spurious, noise-induced, triggers on 'wrong'
// edge by requiring a full clock's worth of samples to be idle
// before accepting trigger
reg watchForTrigger = 0, watchForTrigger_d = 0;
reg [ADC_MUX_SELECT_WIDTH_NONZERO-1:0] triggerLocation;
reg [ADC_RAM_ADDRESS_WIDTH-1:0] triggerDpramAddr;
reg triggered = 0;
always @(posedge adcClk) begin
    watchForTrigger_d <= watchForTrigger;
    if (watchForTrigger) begin
        if (watchForTrigger_d) begin
            if (sampleTriggerValid && (
                sysFallingEdgeTrigger ? !(|sampleBelowTrigger)
                                      : !(|sampleAboveTrigger))) begin
                triggersBeenIdle <= 1;
            end
            if (!triggered) begin
                if (|(sysEventTriggerEnables & eventTriggerStrobes)) begin
                    triggered <= 1;
                    triggerLocation <= 0;
                end
                else if (|triggerFlags && triggersBeenIdle &&
                    triggerFlagsValid) begin
                    triggered <= 1;
                    casex (triggerFlags)
                    8'bxxxxxxx1: triggerLocation <= 0;
                    8'bxxxxxx10: triggerLocation <= 1;
                    8'bxxxxx100: triggerLocation <= 2;
                    8'bxxxx1000: triggerLocation <= 3;
                    8'bxxx10000: triggerLocation <= 4;
                    8'bxx100000: triggerLocation <= 5;
                    8'bx1000000: triggerLocation <= 6;
                    default:     triggerLocation <= 7;
                    endcase
                end
            end
        end
    end
    else begin
        triggersBeenIdle <= 0;
        triggered <= 0;
    end
end

// Acquisition dual-port RAM
reg [ADC_ALL_SAMPLES_WIDTH-1:0] dpram [0:ADC_RAM_CAPACITY-1];
reg [ADC_ALL_SAMPLES_WIDTH-1:0] dpramQ = 0;

// Acquisition state machine
reg dpramWriteEnable = 0;
reg [ADC_RAM_ADDRESS_WIDTH-1:0] dpramWriteAddress = 0;
localparam ACQ_S_START   = 3'd0,
           ACQ_S_FILL    = 3'd1,
           ACQ_S_ARMED   = 3'd2,
           ACQ_S_ACQUIRE = 3'd3,
           ACQ_S_PAUSE   = 3'd4,
           ACQ_S_DONE    = 3'd5;
(*mark_debug=DEBUG*) reg [2:0] acqState = ACQ_S_START;
assign bondedWriteEnableOut = dpramWriteEnable;
assign bondedWriteAddressOut = dpramWriteAddress;
(*mark_debug=DEBUG*) reg                             channelWriteEnable;
(*mark_debug=DEBUG*) reg [ADC_RAM_ADDRESS_WIDTH-1:0] channelWriteAddress;
reg triggerToggle = 0;
(* ASYNC_REG="TRUE" *) reg acqActive_m = 0;
reg acqActive = 0;
reg startMatch = 0;

always @(posedge adcClk) begin
    // DPRAM writes are outside other logic since bonded channels
    // need to be written even though their acqActive is false.
    if (dpramWriteEnable && adcDataValid) begin
        dpramWriteAddress <= dpramWriteAddress + 1;
    end
    channelWriteEnable <= sysIsBonded ? bondedWriteEnableIn:dpramWriteEnable;
    channelWriteAddress <= sysIsBonded ? bondedWriteAddressIn:dpramWriteAddress;
    if (channelWriteEnable && adcDataValid) begin
        dpram[channelWriteAddress] <= adcData;
    end

    //
    // Acquisition state machine
    //
    acqActive_m <= sysAcqActive;
    acqActive   <= acqActive_m;
    if (!acqActive) begin
        // The '-2' is because a counter must reach -1 to assert 'done'
        earlySegmentsCounter <= EARLY_SEGMENTS_COUNT - 2;
        segmentCounter <= sysSegmentCounterLoad;
        acqFinish <= 0;
        acqState <= ACQ_S_START;
    end
    else begin
        case (acqState)
        ACQ_S_START: begin
            dpramWriteEnable <= 1;
            acqCounter <= {1'b0, sysPretriggerLoad};
            acqState <= ACQ_S_FILL;
        end
        ACQ_S_FILL: begin
            if (axiValid) begin
                acqCounter <= acqCounter - 1;
            end
            if (acqCounterDone) begin
                watchForTrigger <= 1;
                acqState <= ACQ_S_ARMED;
            end
        end
        ACQ_S_ARMED: begin
            acqCounter <= sysPosttriggerLoad;
            if (triggered) begin
                triggerToggle = !triggerToggle;
                watchForTrigger <= 0;
                triggerDpramAddr <= dpramWriteAddress;
                dpramWriteEnable <= 1;
                acqState <= ACQ_S_ACQUIRE;
            end
        end
        ACQ_S_ACQUIRE: begin
            // Clock cycles to wait between segments
            segGapCounter <= earlySegmentsCounterDone ? laterSegGapCounterLoad :
                                                        earlySegGapCounterLoad;
            if (axiValid) begin
               acqCounter <= acqCounter - 1;
            end
            if (acqCounterDone) begin
                segmentCounter <= segmentCounter - 1;
                dpramWriteEnable <= 0;
                if (segmentCounterDone) begin
                    acqState <= ACQ_S_DONE;
                end
                else begin
                    acqState <= ACQ_S_PAUSE;
                end
                if (!earlySegmentsCounterDone) begin
                    earlySegmentsCounter <= earlySegmentsCounter - 1;
                end
            end
        end
        ACQ_S_PAUSE: begin
            acqCounter <= sysAcqCounterSegLoad;
            if (segGapCounterDone) begin
                dpramWriteEnable <= 1;
                acqState <= ACQ_S_ACQUIRE;
            end
            else begin
                segGapCounter <= segGapCounter - 1;
            end
        end
        ACQ_S_DONE: begin
            acqFinish <= 1;
        end
        default: ;
        endcase
    end
end

///////////////////////////////////////////////////////////////////////////////
// System Clock Domain

reg [ADC_MUX_SELECT_WIDTH_NONZERO-1:0] sysMuxSelect;
reg [ADC_RAM_ADDRESS_WIDTH-1:0] sysDpramRdAddr;
reg             [ADC_WIDTH-1:0] sysDataMux;

always @(posedge sysClk) begin
    sysAcqFinish_m <= acqFinish;
    sysAcqFinish   <= sysAcqFinish_m;
    if (sysCsrStrobe) begin
        // Must have a valid slice, so must be NONZERO
        sysMuxSelect <= GPIO_OUT[0+:ADC_MUX_SELECT_WIDTH_NONZERO];
        sysDpramRdAddr <= GPIO_OUT[ADC_MUX_SELECT_WIDTH+:ADC_RAM_ADDRESS_WIDTH]
                                                    - TRIGGER_DETECTION_LATENCY;
        sysAcqActive <= GPIO_OUT[31];
        sysFull <= 0;
    end
    else begin
        sysAcqFinish_d <= sysAcqFinish;
        if (sysAcqFinish && !sysAcqFinish_d) begin
            sysAcqActive <= 0;
            sysFull <= 1;
        end
    end
    if (sysTriggerConfigStrobe) begin
        sysTriggerLevel <= GPIO_OUT[ADC_SHIFT+:ADC_WIDTH];
        sysEventTriggerEnables <= GPIO_OUT[16+:TRIGGER_BUS_WIDTH];
        sysFallingEdgeTrigger <= GPIO_OUT[16+TRIGGER_BUS_WIDTH];
        sysSegMode <= GPIO_OUT[30:29];
        sysIsBonded <= GPIO_OUT[31];
    end
    if (sysAcqConfig1Strobe) begin
        sysAcqConfig1 <= GPIO_OUT;
    end
    if (sysAcqConfig2Strobe) begin
        sysAcqConfig2 <= GPIO_OUT;
    end
    // The '-2' is because a counter must reach -1 to assert 'done'
    sysPretriggerLoad = sysPretriggerCount + TRIGGER_DETECTION_LATENCY - 2;
    sysPosttriggerLoad <= {1'b0,
                         (sysSegMode == 0) ? sysContinuousPosttriggerCount :
                        ((sysSegMode == 1) ? sysLongSegmentedPosttriggerCount :
                                             sysShortSegmentedPosttriggerCount)}
                     - TRIGGER_DETECTION_LATENCY - TRIGGER_RESPONSE_LATENCY - 2;
    sysSegmentCounterLoad <= (sysSegMode == 0) ? -1 :
                            ((sysSegMode == 1) ? (LONG_SEGMENT_COUNT - 2) :
                                                 (SHORT_SEGMENT_COUNT - 2));
    sysAcqCounterSegLoad <= (sysSegMode == 1) ?
                         ((LONG_SEGMENT_CAPACITY / AXI_SAMPLES_PER_CLOCK) - 2) :
                         ((SHORT_SEGMENT_CAPACITY / AXI_SAMPLES_PER_CLOCK) - 2);
    dpramQ <= dpram[sysDpramRdAddr];
    sysDataMux <= (SINGLE_SAMPLE_PER_CLOCK)? dpramQ[0+:ADC_WIDTH] :
        dpramQ[sysMuxSelect*ADC_WIDTH+:ADC_WIDTH];
end

wire [7:0] sysSampleWidth = ADC_WIDTH + ADC_SHIFT;
wire       sysSampleSign = 1; // signed
assign sysStatus = { sysAcqActive, sysFull, sysSegMode,
                     {32-1-1-2-3{1'b0}},
                     acqState};
assign sysData = $signed(sysDataMux) << ADC_SHIFT;
assign sysProperties = { {32-8-1{1'b0}},
                         sysSampleSign,
                         sysSampleWidth };

generate
if (SINGLE_SAMPLE_PER_CLOCK) begin
assign sysTriggerLocation = { {32-ADC_WIDTH-ADC_SHIFT{1'b0}},
                                            triggerDpramAddr };
end
else begin
assign sysTriggerLocation = { {32-ADC_WIDTH-ADC_SHIFT{1'b0}},
                                            triggerDpramAddr, triggerLocation };
end
endgenerate

///////////////////////////////////////////////////////////////////////////////
// EVR Clock Domain
(* ASYNC_REG="TRUE" *) reg evrTriggerToggle_m = 0, evrTriggerToggle = 0;
reg evrTriggerMatch = 0;
always @(posedge evrClk) begin
    evrTriggerToggle_m <= triggerToggle;
    evrTriggerToggle   <= evrTriggerToggle_m;
    if (evrTriggerToggle != evrTriggerMatch) begin
        evrTriggerMatch <= !evrTriggerMatch;
        sysTriggerTimestamp <= evrTimestamp;
    end
end

endmodule
