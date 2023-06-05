set syn_prop_dict {
    steps.synth_design.args.directive    {AreaOptimized_high}
    steps.synth_design.args.assert       {1}
    "steps.synth_design.args.more options" {-verbose}
}

set impl_prop_dict {
    steps.opt_design.is_enabled          {1}
    steps.opt_design.args.directive      {Explore}
    steps.opt_design.args.verbose        {1}
    steps.place_design.args.directive    {ExtraTimingOpt}
    steps.phys_opt_design.is_enabled     {1}
    steps.phys_opt_design.args.directive {AggressiveExplore}
    "steps.phys_opt_design.args.more options" {-verbose}
    steps.route_design.args.directive    {AggressiveExplore}
    steps.post_route_phys_opt_design.is_enabled {1}
    steps.post_route_phys_opt_design.args.directive {AggressiveExplore}
    steps.write_bitstream.args.verbose   {1}
}

set_property -dict $syn_prop_dict [get_runs synth_1]
set_property -dict $impl_prop_dict [get_runs impl_1]
