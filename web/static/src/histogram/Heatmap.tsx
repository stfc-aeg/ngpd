import { useEffect, useRef, useState } from "react";
import type { MouseEventHandler } from "react";
import { extent, interpolateInferno, scaleSequentialSqrt } from "d3";

import styles from './style.module.css';
import { ColourBar } from "./Legend";

interface HeatmapProps {
  data: number[][];
}

interface HoverInfo {
  col: number;
  row: number;
  val: number;
  x: number;
  y: number;
}

const Heatmap = ({ data }: HeatmapProps) => {

  const ref = useRef<HTMLCanvasElement>(null);

  const [hovered, setHovered] = useState<HoverInfo | null>(null);

  const [min, max] = extent(data.flat());

  const { numCols, numRows } = {
    numCols: data[0]?.length ?? 0,
    numRows: data?.length ?? 0
  }

  const colour = scaleSequentialSqrt(interpolateInferno).domain([min ?? 0, max ?? 1024]);

  const handleMouseMove: MouseEventHandler<HTMLCanvasElement> = (e) => {
    const canvas = e.currentTarget;
    const rect = canvas.getBoundingClientRect();

    const col = Math.floor((e.clientX - rect.left) / (rect.width / numCols));
    const row = Math.floor((e.clientY - rect.top) / (rect.height / numRows));

    if (row < 0 || row >= numRows || col < 0 || col >= numCols) {
      setHovered(null);
      return;
    }
    setHovered({ col, row, x: e.clientX - rect.left, y: e.clientY - rect.top, val: data[row][col] });
  }

  useEffect(() => {
    const start = Date.now();
    const canvas = ref.current;
    const ctx = canvas?.getContext("2d", {alpha: false});
    const colour = scaleSequentialSqrt(interpolateInferno).domain([min ?? 0, max ?? 1024]);
    if (canvas && ctx) {
      const cellWidth = canvas.width / numCols;
      const cellHeight = canvas.height / numRows;

      ctx.clearRect(0, 0, ctx.canvas.clientWidth, ctx.canvas.clientHeight);
      // fill the "background" with the 0 value.
      // this way we don't need to create a rect for every cell that is 0, speeding up the rendering of the canvas significantly
      ctx.fillStyle = colour(0);
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      for (const [i, row] of data.entries()) {
        for (const [j, d] of row.entries()) {
          if (d != 0) {
            ctx.fillStyle = colour(d);
            // The +1 to width and height is to avoid graphical glitches, so each cell very slightly overlaps the ones next to it
            ctx.fillRect(j * cellWidth, i * cellHeight, cellWidth + 1, cellHeight + 1);
          }
        }
      }
    }
    const ms = Date.now() - start;
    console.log(`Rendering graph took ${ms}ms`)
  }, [numCols, numRows, data, min, max]);

  return (
    <>
      <canvas id="heatmapCanvas" ref={ref} className={styles.heatmapCanvas} onMouseMove={handleMouseMove} onMouseLeave={() => setHovered(null)} height={512} width={1024}/>
      <ColourBar colourScale={colour} min={min} max={max} />
      {hovered &&
        <div className={styles.tooltip} style={{ left: hovered.x, top: hovered.y }}>
          {`(${hovered.col}, ${hovered.row}): Val: ${hovered.val}`}
        </div>
      }
    </>
  )
}


export { Heatmap };