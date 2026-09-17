type ChannelIndex = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7;
type FilterTypes = "rectangle" | "gaussian" | "exponential" | "trapezoidal"


type DeviceTree = {
  base_ip: string;
  num_cards: number;
  dummy_system: string;
  connect: boolean;
}

type MonitorTree = {
  adc: {
    temperature: number;
    voltages: {[key: string]: number};
    trip_temp: number;
  };
  preamp: {
    temperature: number;
    trip_temp: number;
  };
  fpga: {
    temperature: number;
    voltages: {[key: string]: number};
  }
}

type ConfigAnalogTree = {
  gain: number;
  offset: number;
}

type ConfigBaseSubTree = {
  use_fixed: boolean;
  fixed: number;
  error_limit: number;
  div_cont: number;
}

type ConfigFilterTree = {
  type: FilterTypes;
  arg_1: number;
  arg_2: number;
  arg_float: number;
}

type ConfigDiscrimTree = {
  height_min: number;
  height_max: number;
  adaptive: boolean;
  enable_tail_sum: boolean;
  enable_fall_time: boolean;
  min_fall: number;
  max_fall: number;
  min_count: number;
  threshold_c: number;
  threshold_m: number;
}

type ConfigTailMeasureTree = {
  delay: number;
  num_sample: number;
  fall_time_frac: number;
  enable_tail_subtract: number;
  enable_subtract_test: number;
  enable_subtract_neutron: number;
}

type ConfigTriggerTree = {
  threshold: number;
  separation: number;
  data_delay: number;
  trig_delay: number;
  delay_a: number;
  delay_b: number;
  width_a: number;
  width_b: number;
}

type ConfigTree = {
  analog: Record<`channel_${ChannelIndex}`, ConfigAnalogTree>;
  base_sub: Record<`channel_${ChannelIndex}`, ConfigBaseSubTree>;
  filter: Record<`channel_${ChannelIndex}`, ConfigFilterTree>;
  discrimination: Record<`channel_${ChannelIndex}`, ConfigDiscrimTree>;
  tail_measure: Record<`channel_${ChannelIndex}`, ConfigTailMeasureTree>;
  trigger: Record<`channel_${ChannelIndex}`, ConfigTriggerTree>;
  playback: {
    enabled: boolean;
    file_name: string;
  };
  histogram: {
    num_bins_height: number;
    num_bins_tailsum: number;
    shift_height: number;
    shift_tailsum: number;
    separate_ngp: boolean;
  }
}

type GraphTree = {
  data_shape: number[];
  hist: string;
  tailsum: string;
  pulse_height: string;
  refresh_data: null;
  channel: ChannelIndex;
  signal: string;
}

type AcquireTree = {
  num_cycles: number;
  frame_length: number;
  scope_setup: boolean;
  scope_run: boolean;
  run: null;
  state: {
    total: number;
    current: number;
    status: string;
  }
  graph: GraphTree;
}

export interface EndpointParams {
  /* Add any Parameters you'll be using to this interface, such as this example*/
  device: DeviceTree;
  monitor: MonitorTree;
  config: ConfigTree;
  acq: AcquireTree;
}

export const channels: readonly ChannelIndex[] = [0, 1, 2, 3, 4, 5, 6, 7];