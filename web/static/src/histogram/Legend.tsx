import type { ScaleSequentialBase } from "d3"
import { useMemo, useRef } from "react";

interface ColourBarProps {
  colourScale: ScaleSequentialBase<string>;
  min?: number;
  max?: number;
  step?: number;
}

const ColourBar = ({ colourScale, min = 0, max = 1024, step = 100 }: ColourBarProps) => {
  // TODO: WORK IN PROGRESS
  const ref = useRef<HTMLCanvasElement>(null);

  const stops = useMemo(() => {
    const stops = [];
    for (let i = min; i <= max; i+=step) {
      stops.push(colourScale(i));
    }
    return stops;
  }, [colourScale, min, max, step]);

  const gradient = `linear-gradient(90deg, ${stops.join(", ")})`;

  return (
    <div style={{width: "100%", height: "1rem", background: gradient, marginTop: "50px"}}/>
  )

}

export { ColourBar }