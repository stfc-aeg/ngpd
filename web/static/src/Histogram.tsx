import type { AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./types";
import { useEffect, useMemo, useState, type ComponentProps, type MouseEventHandler } from "react";
import { getDimensions } from "./histogram/util";
import styles from "./histogram/style.module.css";
import { Axes, Heatmap, Scatter } from "./histogram";

interface HistogramProps {
  endpoint: AdapterEndpoint<EndpointParams>;
  data_endpoint?: AdapterEndpoint<{ value: EndpointParams["acq"]["graph"]["hist"] }>;
}

const Histogram = ({ endpoint, data_endpoint }: HistogramProps) => {

  const [min_height, setMinHeight] = useState(0);
  const [max_height, setMaxHeight] = useState(0);
  const [tailsum_left, setTailsumLeft] = useState(0);
  const [tailsum_right, setTailsumRight] = useState(0);

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

  const data = useMemo(() => {
    const hist = data_endpoint ? data_endpoint.data?.value : endpoint.data?.acq.graph.hist;

    const reshaped_data: number[][] = [];

    if (hist) {
      try {
        const tmp = Uint8Array.fromBase64(hist);
        const buf = new Uint32Array(tmp.buffer);

        const dataset_length = numCols * numRows;

        for (let i = 0; i < dataset_length; i += numRows) {
          reshaped_data.push(Array.from(buf.slice(i, i + numCols)))
        }
      } catch (err) {
        console.log(err);
      }
    }

    return reshaped_data;
  }, [data_endpoint, endpoint.data?.acq.graph.hist, numCols, numRows]);

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
    const newMinHeight = endpoint.data?.config.discrimination[`channel_${chan}`].height_min ?? 0;
    const newMaxHeight = endpoint.data?.config.discrimination[`channel_${chan}`].height_max ?? 0;
    const newTailsumLeft = endpoint.data?.config.discrimination[`channel_${chan}`].threshold_c ?? 0;
    const newTailsumRight = endpoint.data?.config.discrimination[`channel_${chan}`].threshold_m ?? 0;

    setMinHeight(newMinHeight >> height_shift);
    setMaxHeight(newMaxHeight >> height_shift);
    setTailsumLeft(newTailsumLeft >> tailsum_shift);
    setTailsumRight((newTailsumLeft >> tailsum_shift) + ((newTailsumRight / (2 ** tailsum_shift)) * 65536));
  }, [chan, endpoint.data?.config.discrimination, height_shift, tailsum_shift]);


  return (
    <div className={styles.heatmap}>
      <Axes xRange={[0, numCols]} yRange={[0, numRows]} step={128} xLabel="Pulse Height" yLabel="Tail Sum" />
      <Scatter data={scatterData} xRange={[0, numCols]} yRange={[0, numRows]} onMouseDrag={onScatterDrag} onMouseUp={onScatterRelease} />
      <Heatmap data={data} />
    </div>
  )
}

export { Histogram }