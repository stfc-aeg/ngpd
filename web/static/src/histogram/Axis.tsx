import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { getDimensions } from "./util";
import { range, scaleLinear } from "d3";

import styles from './style.module.css';

interface AxisSvgProps {
  xRange: [min: number, max: number];
  yRange: [min: number, max: number];
  xLabel?: string;
  yLabel?: string;
  step?: number;
}

const Axes = ({ xRange, yRange, xLabel, yLabel, step = 100 }: AxisSvgProps) => {

  const ref = useRef<SVGSVGElement>(null);

  const [dims, setDims] = useState<DOMRect | undefined>(undefined);

  // using linear scales to match data range to pixel range of graph
  const xScale = scaleLinear(xRange, [0, dims?.width ?? 0]);
  const yScale = scaleLinear(yRange, [0, dims?.height ?? 0]);

  const xTicks = range(xRange[0], xRange[1] + 1, step).map(
    (value) => ({
      value, offset: xScale(value)
    })
  );
  const yTicks = range(yRange[0], yRange[1] + 1, step).map(
    (value) => ({
      value, offset: yScale(value)
    })
  );

  const handleResize = () => {
    setDims(getDimensions(ref.current));
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

  useLayoutEffect(() => {
    setDims(getDimensions(ref.current));
  }, []);

  return (
    <svg ref={ref} className={styles.axesSvg}>
      <g>
        <path // Vertical Line
          d={["M", -1, 0, "L", -1, dims?.height ?? 0].join(" ")}
          fill="none"
          stroke="currentColor"
        />
        {yTicks.map(({ value, offset }) => (
          <g key={value} transform={`translate(0, ${offset})`}>
            <line x2={-6} stroke="currentColor" />
            <text className={styles.tickText}
              style={{ transform: "translateX(-20px)" }}
            >
              {value}
            </text>
          </g>
        ))}
        <text className={styles.yTitle} x={-(dims?.height ?? 0) / 2} y={-40}>
          {yLabel}
        </text>
      </g>
      <g transform={`translate(0, ${dims?.height ?? 0})`}>
        <path // horizontal axis
          d={["M", 0, 0, "L", dims?.width ?? 0, 0].join(" ")}
          fill="none"
          stroke="currentColor"
        />
        {xTicks.map(({ value, offset }) => (
          <g key={value} transform={`translate(${offset}, 0)`}>
            <line y2={6} stroke="currentColor" />
            <text className={styles.tickText}
              style={{ transform: "translateY(20px)" }}
            >
              {value}
            </text>
          </g>
        ))}
        <text className={styles.xTitle} x={(dims?.width ?? 0) / 2} y={50}>
          {xLabel}
        </text>
      </g>
    </svg>
  )
}

export { Axes };