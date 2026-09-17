const getDimensions = (target?: SVGSVGElement | HTMLElement | null) => {
  const siblings = target?.parentElement?.children;
  if (siblings) {
    for (const sib of siblings) {
      if (sib.id == "heatmapCanvas") {
        return sib.getBoundingClientRect()
      }
    }
    console.warn("No Canvas in Siblings");
    return;
  }
  console.warn("No Siblings");
}

export { getDimensions }