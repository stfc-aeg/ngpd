import { EndpointButton, EndpointCheckbox, EndpointDropdown, EndpointInput, EndpointSelect, OdinGraph, TitleCard, useError, type AdapterEndpoint } from "@dssg/odin-react";
import { useEffect, useState, useTransition } from "react";
import { ButtonGroup, Col, Container, Dropdown, DropdownButton, DropdownItem, FloatingLabel, Form, InputGroup, OverlayTrigger, ProgressBar, Row, Tooltip, type DropdownButtonProps } from "react-bootstrap";
import { Histogram } from "./Histogram";
import { channels, type EndpointParams, type DatasetName } from "./types";
import { Floppy, Download, ArrowClockwise } from "react-bootstrap-icons";

interface GraphPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}

const GraphPage = ({ endpoint }: GraphPageProps) => {

  const { setError } = useError();
  const [tailsum_data, setTailsum_data] = useState<number[][]>([]);
  const [pulseheight_data, setPulseheight_data] = useState<number[][]>([]);
  const [selected_dataset, setSelectedDataset] = useState<DatasetName>("all");
  const [downloadPending, startTransition] = useTransition();

  const shape = endpoint.data?.acq.graph.data_shape ?? [1, 1024, 1024];
  const { height_bins, tailsum_bins } = {
    height_bins: shape[2],
    tailsum_bins: shape[1]
  }

  useEffect(() => {  // Fetch Tailsum Graph Data
    const fetch_data = async () => {
      const tailsum = (await endpoint.get<ArrayBuffer>("data/tailsum", { responseType: "arraybuffer" }));
      const reshaped_data: number[][] = [];
      if (tailsum) {
        try {
          const buf = new Uint32Array(tailsum);

          for (let i = 0; i < buf.length; i += tailsum_bins) {
            reshaped_data.push(Array.from(buf.slice(i, i + tailsum_bins)))
          }

        } catch (err) {
          console.error(err);
        }
      }

      setTailsum_data(reshaped_data);
    }

    fetch_data();


    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [height_bins, tailsum_bins, endpoint.data?.acq.graph.channel, endpoint.data?.acq.graph.refresh_data])

  useEffect(() => {  // Fetch Pulse Height Graph Data
    const fetch_data = async () => {
      const pulseheight = (await endpoint.get<ArrayBuffer>("data/pulse_height", { responseType: "arraybuffer" }));
      const reshaped_data: number[][] = [];
      if (pulseheight) {
        try {
          const buf = new Uint32Array(pulseheight);

          for (let i = 0; i < buf.length; i += height_bins) {
            reshaped_data.push(Array.from(buf.slice(i, i + height_bins)))
          }

        } catch (err) {
          console.error(err);
        }
      }

      setPulseheight_data(reshaped_data);
    }

    fetch_data();


    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [height_bins, tailsum_bins, endpoint.data?.acq.graph.channel, endpoint.data?.acq.graph.refresh_data])

  const selectSignal: DropdownButtonProps["onSelect"] = (eventKey) => {
    if(eventKey) {
      setSelectedDataset(eventKey as DatasetName);
    }
  }

  const create_file_name = () => {
    const now = new Date();
    const timestamp = now.toISOString();

    return ["ngpd_hist", timestamp, "h5"].join(".");

  }

  const download_file = async (filename: string) => {
    try {
      const file = await endpoint.get<Blob>("data/download", { responseType: "blob" });
      const url = URL.createObjectURL(file);
      const link = document.createElement("a");
      link.href = url;
      link.setAttribute("download", filename);
      document.body.appendChild(link);

      link.click();

      // cleanup
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
    } catch (err) {
      setError(err instanceof Error ? err : Error("Unknown Error Occurred"));
    }
  }

  const Controls = (
    <Row>
      <Col>
        <InputGroup>
          <EndpointDropdown endpoint={endpoint} fullpath="acq/graph/channel" title={`Channel ${endpoint.data?.acq.graph.channel}`} variant="secondary">
            {channels.map((chan) => (
              <DropdownItem eventKey={chan} key={chan} active={endpoint.data?.acq.graph.channel == chan}>{chan}</DropdownItem>
            ))}
          </EndpointDropdown>
          <DropdownButton title={`Signal: ${selected_dataset}`} onSelect={selectSignal}>
            <Dropdown.Item eventKey="all">All</Dropdown.Item>
            <Dropdown.Item eventKey="neutron">Neutron</Dropdown.Item>
            <Dropdown.Item eventKey="gamma">Gamma</Dropdown.Item>
            <Dropdown.Item eventKey="pileup">Pileup</Dropdown.Item>
          </DropdownButton>
        </InputGroup>
      </Col>
      <Col>
        <h3>Histogram</h3>
      </Col>
      <Col xs="auto">
        <ButtonGroup>
          <OverlayTrigger placement="auto" overlay={<Tooltip>Refresh the data, reading from the device.</Tooltip>}>
            <EndpointButton endpoint={endpoint} fullpath="acq/graph/refresh_data" value={true}>
              <ArrowClockwise/>
            </EndpointButton>
          </OverlayTrigger>
          <OverlayTrigger placement="auto" overlay={<Tooltip>Save the Histogram data to a HDF5 file.</Tooltip>}>
            <EndpointButton endpoint={endpoint} fullpath="acq/graph/save" disabled={downloadPending} pre_method={create_file_name} variant="secondary">
              <Floppy/>
            </EndpointButton>
          </OverlayTrigger>
          <OverlayTrigger placement="auto" overlay={<Tooltip>Save the Histogram data to a HDF5 file, and download it.</Tooltip>}>
            <EndpointButton endpoint={endpoint} fullpath="acq/graph/save" disabled={downloadPending} variant="secondary"
              pre_method={create_file_name} post_method={({ value }: { value?: string }) => startTransition(async () => download_file(value!))}
              post_args={{ value: undefined }} >
              {downloadPending ? "Downloading..." : <Download/>}
            </EndpointButton>
          </OverlayTrigger>
        </ButtonGroup>
      </Col>
    </Row>
  )

  return (
    <Container>
      <Row>
        <Col>
          <TitleCard title={Controls}>
            <Row>
              <Col>
                <Histogram endpoint={endpoint} selected_dataset={selected_dataset}/>
              </Col>
            </Row>
            <Row>
              <Col xs="12" md="12" lg="6">
                <OdinGraph data={pulseheight_data} series_names={["Neutrons", "Gamma", "Pileup", "All"]} title="Pulse Height" />
              </Col>
              <Col xs="12" md="12" lg="6">
                <OdinGraph data={tailsum_data} series_names={["Neutrons", "Gamma", "Pileup", "All"]} title="Tail Sum" />
              </Col>
            </Row>
          </TitleCard>
        </Col>
      </Row>
      <Row>
        <Col>
          <TitleCard title="Histogram Controls">
            <Row>
              <Col>
                <Row>
                  <Form.Label column>Pulse Height</Form.Label>
                  <Col xxl={{ order: "last", span: "auto" }} xl={{ order: "first" }}>
                    <div style={{ paddingTop: "26px" }} />
                  </Col>
                </Row>
              </Col>
              <Col>
                <Row>
                  <Form.Label column>Tail Sum</Form.Label>
                  <Col xxl={{ order: "last", span: "auto" }} xs={{ order: "first" }} style={{ alignContent: "center" }}><EndpointCheckbox endpoint={endpoint} fullpath="config/histogram/separate_ngp" type="switch" label="Separate NGP" /></Col>
                </Row>
              </Col>
            </Row>
            <Row>
              <Col>
                <InputGroup>
                  <FloatingLabel label="Bit Shift">
                    <EndpointInput endpoint={endpoint} fullpath="config/histogram/shift_height" size="sm" />
                  </FloatingLabel>
                  <FloatingLabel label="Num Bins">
                    <EndpointSelect endpoint={endpoint} fullpath="config/histogram/num_bins_height" />
                  </FloatingLabel>
                </InputGroup>
              </Col>
              <Col>
                <InputGroup>
                  <FloatingLabel label="Bit Shift">
                    <EndpointInput endpoint={endpoint} fullpath="config/histogram/shift_tailsum" size="sm" />
                  </FloatingLabel>
                  <FloatingLabel label="Num Bins">
                    <EndpointSelect endpoint={endpoint} fullpath="config/histogram/num_bins_tailsum" />
                  </FloatingLabel>
                </InputGroup>
              </Col>
            </Row>
          </TitleCard>
        </Col>
        <Col lg="6" md="12">
          <TitleCard title="Run Controls">
            <Row>
              <Col>
                <FloatingLabel label="Num Cycles">
                  <EndpointInput endpoint={endpoint} fullpath="acq/num_cycles" />
                </FloatingLabel>
              </Col>
              <Col>
                <FloatingLabel label="Frame Length (seconds)">
                  <EndpointInput endpoint={endpoint} fullpath="acq/frame_length" />
                </FloatingLabel>
              </Col>


            </Row>
            <Row>
              <Col xl="4" className="mt-2">
                <Row>
                  <Col>
                    <EndpointCheckbox endpoint={endpoint} fullpath="acq/scope_setup" type="switch" label="Setup Scope" />
                  </Col>
                  <Col>
                    <EndpointCheckbox endpoint={endpoint} fullpath="acq/scope_run" type="switch" label="Run Scope" />
                  </Col>
                </Row>
              </Col>
              <Col xl="8" className="mt-2">
                <InputGroup>
                  <FloatingLabel label="Playback File">
                    <EndpointSelect endpoint={endpoint} fullpath={"config/playback/file_name"} />
                  </FloatingLabel>
                  <EndpointButton endpoint={endpoint} fullpath="config/playback/enabled" value={!endpoint.data?.config.playback.enabled}
                    variant={endpoint.data?.config.playback.enabled ? "primary" : "outline-primary"}>
                    {endpoint.data?.config.playback.enabled ? "Disable Playback" : "Enable Playback"}
                  </EndpointButton>
                </InputGroup>
              </Col>
            </Row>
            <Row className="mt-2">
              <Col>
                <ButtonGroup size="lg" style={{ width: "100%" }}>
                  <EndpointButton endpoint={endpoint} fullpath="acq/run" value={true} disabled={endpoint.data?.acq.state.status == "running"}>
                    Start
                  </EndpointButton>
                  <EndpointButton endpoint={endpoint} fullpath="acq/run" value={false} disabled={endpoint.data?.acq.state.status != "running"} variant="danger">
                    Stop
                  </EndpointButton>
                </ButtonGroup>
              </Col>
            </Row>
            <Row className="mt-2">
              <Col>
                <ProgressBar label={endpoint.data?.acq.state.status} now={endpoint.data?.acq.state.current} max={endpoint.data?.acq.state.total} />
              </Col>
            </Row>
          </TitleCard >
        </Col >
      </Row >
    </Container >
  )
}

export { GraphPage };
