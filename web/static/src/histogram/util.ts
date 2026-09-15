const getDimensions = (target?: SVGSVGElement | HTMLElement | null) => {
  const siblings = target?.parentElement?.children;
  if (siblings) {
    for (const sib of siblings) {
      if (sib.id == "heatmapCanvas") {
        return sib.getBoundingClientRect()
      }
    }
    console.log("No Canvas in Siblings");
    return;
  }
}

export { getDimensions }