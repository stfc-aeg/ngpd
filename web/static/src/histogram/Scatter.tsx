import { type MouseEventHandler, type CSSProperties, useRef, useState, useEffect } from "react";
import { line, scaleLinear, scaleOrdinal, schemeObservable10 } from "d3";

import styles from './style.module.css';
import { getDimensions } from "./util";

interface ScatterProps {
  data: Record<string, Array<[number, number]>>;
  colors?: readonly string[];
  xRange: [min: number, max: number];
  yRange: [min: number, max: number];
  onMouseDrag?: MouseEventHandler<SVGCircleElement | SVGRectElement>;
  onMouseUp?: MouseEventHandler<SVGCircleElement | SVGRectElement>;
}

type HoverInfo = {
  col: number;
  row: number;
  x: number;
  y: number;
  name: string;
  color?: CSSProperties["color"];
}

const Scatter = ({ data, xRange, yRange, onMouseDrag, onMouseUp, colors = schemeObservable10 }: ScatterProps) => {

  const ref = useRef<SVGSVGElement>(null);
  const [hovered, setHovered] = useState<HoverInfo | null>(null);
  const [dims, setDims] = useState<DOMRect | undefined>(undefined);
  const [selectedCircle, setCircle] = useState<SVGCircleElement | null>(null);

  // using linear scales to match data range to pixel range of graph
  const xScale = scaleLinear(xRange, [0, dims?.width ?? 0]);
  const yScale = scaleLinear(yRange, [0, dims?.height ?? 0]);

  const colourScale = scaleOrdinal(Object.keys(data), colors);

  const lineBuilder = line((d) => xScale(d[0]), (d) => yScale(d[1]));

  const handleResize = () => {
    setDims(getDimensions(ref.current));
  }

  const setHoverFromCircle = (circle: SVGCircleElement) => {
    const x = circle.cx.baseVal.value;
    const y = circle.cx.baseVal.value;
    const col = xScale.invert(x);
    const row = yScale.invert(y);
    const name = circle.parentElement?.id ?? "unknown";

    setHovered({
      col, row, name, x, y,
      color: colourScale(name)
    })
  }

  const mouseEnter: MouseEventHandler<SVGCircleElement> = (e) => {
    const circle = e.currentTarget;
    setHoverFromCircle(circle);
  }

  const mouseLeave: MouseEventHandler<SVGCircleElement> = (e) => {
    setHovered(null);
  }

  const mouseDown: MouseEventHandler<SVGCircleElement> = (e) => {
    setCircle(e.currentTarget);
  }

  const mouseUp: MouseEventHandler<SVGCircleElement | SVGRectElement> = (e) => {
    setCircle(null);
    if (onMouseUp && selectedCircle) {
      e.currentTarget = selectedCircle;
      onMouseUp(e);
    }
  }

  const mouseMove: MouseEventHandler<SVGGeometryElement> = (e) => {
    if (e.buttons & 1 && selectedCircle) {
      // mouse click is held down and we've selected a circle from one of the datasets
      setHoverFromCircle(selectedCircle);
      if (onMouseDrag) {
        e.currentTarget = selectedCircle;
        onMouseDrag(e as React.MouseEvent<SVGCircleElement>);
      }
    }
  }

  useEffect(() => {
    const svg = ref.current;
    window.addEventListener("resize", handleResize);
    svg?.addEventListener("load", handleResize);

    return () => {
      window.removeEventListener("resize", handleResize);
      svg?.addEventListener("load", handleResize);
    }
  }, []);

  return (
    <>
      <svg ref={ref} className={styles.scatterSvg}>
        {
          Object.entries(data).map(([key, line_data]) => (
            <g key={key} id={key}>
              <path
                d={lineBuilder(line_data) ?? ""}
                stroke={colourScale(key)}
                strokeDasharray="10 2 5 2"
                strokeOpacity={0.7}
                fill="none"
                strokeWidth={4}
              />
              {
                line_data.map((coord, i) => (
                  <circle key={i}
                    cx={xScale(coord[0])}
                    cy={yScale(coord[1])}
                    r={10}
                    stroke={colourScale(key)}
                    fill={colourScale(key)}
                    onMouseEnter={mouseEnter}
                    onMouseLeave={mouseLeave}
                    onMouseMove={onMouseDrag}
                    onMouseUp={mouseUp}
                    onMouseDown={mouseDown}
                  />
                ))
              }
            </g>
          ))
        }
        {selectedCircle &&
          <rect x={-40} y={-20}
            width={"100%"} height={"100%"}
            fill="transparent"
            onMouseMove={mouseMove}
            onMouseUp={mouseUp}
          />
        }
      </svg>
      {hovered &&
        <div className={styles.tooltip} style={{ left: hovered.x, top: hovered.y, border: `5px solid ${hovered.color}` }}>
          {`${hovered.name}: (${hovered.col}, ${hovered.row})`}
        </div>
      }
    </>
  )
}

export { Scatter }
