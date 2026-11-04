import type { AdapterEndpoint } from "@dssg/odin-react";
import { useEffect, useState, type ComponentProps, type MouseEventHandler } from "react";
import { Axes, Heatmap, Scatter } from "./histogram";
import styles from "./histogram/style.module.css";
import { getDimensions } from "./histogram/util";
import type { EndpointParams, DatasetName } from "./types";

interface HistogramProps {
  endpoint: AdapterEndpoint<EndpointParams>;
  selected_dataset: DatasetName;
}



const Histogram = ({ endpoint, selected_dataset }: HistogramProps) => {

  const [min_height, setMinHeight] = useState(0);
  const [max_height, setMaxHeight] = useState(0);
  const [tailsum_left, setTailsumLeft] = useState(0);
  const [tailsum_right, setTailsumRight] = useState(0);
  const [hist_data, setHist_data] = useState<Record<DatasetName, number[][]> | null>(null);

  const shape = endpoint.data?.acq.graph.data_shape ?? [1, 1024, 1024];
  const chan = endpoint.data?.acq.graph.channel ?? 0;
  const height_shift = endpoint.data?.config.histogram.shift_height ?? 0;
  const tailsum_shift = endpoint.data?.config.histogram.shift_tailsum ?? 0;

  const { numCols, numRows } = {
    numCols: shape[1],
    numRows: shape[2]
  }

  const scatterData: ComponentProps<typeof Scatter>["data"] = {
    "Min Height": [[min_height, 0], [min_height, numRows]],
    "Max Height": [[max_height, 0], [max_height, numRows]],
    "Tail Sum": [[0, tailsum_left], [numCols, tailsum_right]]
  }

  useEffect(() => {
    const fetch_data = async () => {
      const hist = (await endpoint.get<ArrayBuffer>("data/hist", { responseType: "arraybuffer" }));
      const dataset_names: DatasetName[] = ["neutron", "gamma", "pileup", "all"];
      const datasets: Record<string, number[][]> = {};
      if (hist) {
        try {

          const buf = new Uint32Array(hist);
          
          const dataset_length = numCols * numRows;
          let reshaped_data: number[][] = [];
          for(let j = 0; j < buf.length; j+= dataset_length){
            reshaped_data = [];
            for (let i = j; i < j+dataset_length; i += numRows) {
              reshaped_data.push(Array.from(buf.slice(i, i + numCols)))
            }
            datasets[dataset_names[j / dataset_length]] = reshaped_data;
        }
        } catch (err) {
          console.log(err);
        }
      }
      setHist_data(datasets);
    }

    fetch_data();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [chan, numCols, numRows, endpoint.data?.acq.graph.refresh_data]);

  const onScatterDrag: MouseEventHandler<SVGCircleElement> = (e) => {
    if (e.buttons & 1) { // test that left click is held
      const data_name = e.currentTarget.parentElement?.id;
      const parentSvg = e.currentTarget.parentElement?.parentElement;
      const rect = getDimensions(parentSvg);

      if (rect && data_name) {
        const col = Math.floor((e.clientX - rect.left) / (rect.width / numCols));
        const row = Math.floor((e.clientY - rect.top) / (rect.height / numRows));

        switch (data_name) {
          case "Max Height":
            if (col > 0 && col <= numCols) {
              setMaxHeight(col);
              if (!(col > min_height)) {
                setMinHeight(col - 1);
              }
            }
            break;
          case "Min Height":
            if (col < numCols && col >= 0) {
              setMinHeight(col);
              if (!(col < max_height)) {
                setMaxHeight(col + 1);
              }
            }
            break;
          case "Tail Sum":
            if (row < numRows && row >= 0) {
              const x = e.currentTarget.cx.baseVal.value;
              if (x == 0) {
                const tailsum_diff = tailsum_right - tailsum_left;
                setTailsumLeft(row);
                setTailsumRight(row + tailsum_diff);
              } else {
                setTailsumRight(row);
                if (!(row > tailsum_left)) {
                  setTailsumLeft(row);
                }
              }
            }
            break;
        }
      }
    }
  }

  const onScatterRelease: MouseEventHandler<SVGCircleElement> = (e) => {
    const data_name = e.currentTarget.parentElement?.id;
    console.log("Putting for ", data_name);
    switch (data_name) {
      case "Max Height":
      case "Min Height":
        endpoint.put({
          "height_max": max_height << height_shift,
          "height_min": min_height << height_shift
        }, `config/discrimination/channel_${chan}`);
        break;
      case "Tail Sum":
        endpoint.put({
          "threshold_c": tailsum_left << tailsum_shift,
          "threshold_m": (2 ** tailsum_shift) * ((tailsum_right - tailsum_left) / 65536)
        }, `config/discrimination/channel_${chan}`);
        break;
    }
  }

  useEffect(() => {
    const updateScatter = async () => {
      const newMinHeight = endpoint.data?.config.discrimination[`channel_${chan}`].height_min ?? 0;
      const newMaxHeight = endpoint.data?.config.discrimination[`channel_${chan}`].height_max ?? 0;
      const newTailsumLeft = endpoint.data?.config.discrimination[`channel_${chan}`].threshold_c ?? 0;
      const newTailsumRight = endpoint.data?.config.discrimination[`channel_${chan}`].threshold_m ?? 0;

      setMinHeight(newMinHeight >> height_shift);
      setMaxHeight(newMaxHeight >> height_shift);
      setTailsumLeft(newTailsumLeft >> tailsum_shift);
      setTailsumRight((newTailsumLeft >> tailsum_shift) + ((newTailsumRight / (2 ** tailsum_shift)) * 65536));
    }
    updateScatter();
  }, [chan, endpoint.data?.config.discrimination, height_shift, tailsum_shift, endpoint.data?.acq.graph.refresh_data]);


  return (
    <div className={styles.heatmap}>
      <Axes xRange={[0, numCols]} yRange={[0, numRows]} step={128} xLabel="Pulse Height" yLabel="Tail Sum" />
      <Scatter data={scatterData} xRange={[0, numCols]} yRange={[0, numRows]} onMouseDrag={onScatterDrag} onMouseUp={onScatterRelease} />
      <Heatmap data={hist_data?.[selected_dataset] ?? []} />
    </div>
  )
}

export { Histogram };
