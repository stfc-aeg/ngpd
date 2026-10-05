import { EndpointButton, EndpointCheckbox, EndpointDropdown, EndpointInput, EndpointSelect, OdinGraph, TitleCard, useAdapterEndpoint, type AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./types";
import { Badge, Button, ButtonGroup, Col, Container, DropdownItem, FloatingLabel, Form, InputGroup, ProgressBar, Row, Stack } from "react-bootstrap";
import { Histogram } from "./Histogram";
import { useEffect, useMemo, useState } from "react";

interface GraphPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
  data_endpoint: AdapterEndpoint<{ value: EndpointParams["acq"]["graph"]["hist"] }>;
}

const GraphPage = ({ endpoint, data_endpoint }: GraphPageProps) => {


  const [tailsum_data, setTailsum_data] = useState<number[][]>([]);
  const [pulseheight_data, setPulseheight_data] = useState<number[][]>([]);

  const shape = endpoint.data?.acq.graph.data_shape ?? [1, 1024, 1024];
  const { numCols, numRows } = {
    numCols: shape[1],
    numRows: shape[2]
  }

  useEffect(() => {
    const fetch_data = async () => {
      const tailsum = (await endpoint.get<{ "value": string }>("acq/graph/tailsum")).value;
      const reshaped_data: number[][] = [];
      if (tailsum) {
        try {
          const tmp = Uint8Array.fromBase64(tailsum);
          const buf = new Uint32Array(tmp.buffer);
          console.log(`Tailsum Buf Length: ${buf.length}`);

          for (let i = 0; i < buf.length; i += numRows) {
            reshaped_data.push(Array.from(buf.slice(i, i + numCols)))
          }

        } catch (err) {
          console.error(err);
        }
      }

      setTailsum_data(reshaped_data);
    }

    fetch_data();


    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [numCols, numRows, data_endpoint.data?.value])

  useEffect(() => {
    const fetch_data = async () => {
      const pulseheight = (await endpoint.get<{ "value": string }>("acq/graph/pulse_height")).value;
      const reshaped_data: number[][] = [];
      if (pulseheight) {
        try {
          const tmp = Uint8Array.fromBase64(pulseheight);
          const buf = new Uint32Array(tmp.buffer);
          console.log(`PulseHeight Buf Length: ${buf.length}`);

          for (let i = 0; i < buf.length; i += numCols) {
            reshaped_data.push(Array.from(buf.slice(i, i + numRows)))
          }

        } catch (err) {
          console.error(err);
        }
      }

      setPulseheight_data(reshaped_data);
    }

    fetch_data();


    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [numCols, numRows, data_endpoint.data?.value])

  const Controls = (
    <Row>
      <Col>
        <InputGroup>
          <EndpointDropdown endpoint={endpoint} fullpath="acq/graph/channel" title={`Channel ${endpoint.data?.acq.graph.channel}`} variant="secondary">
            {[0, 1, 2, 3, 4, 5, 6, 7].map((chan) => (
              <DropdownItem eventKey={chan} active={endpoint.data?.acq.graph.channel == chan}>{chan}</DropdownItem>
            ))}
          </EndpointDropdown>
          <EndpointDropdown endpoint={endpoint} fullpath="acq/graph/signal" title={`Signal: ${endpoint.data?.acq.graph.signal}`} />
        </InputGroup>
      </Col>
      <Col>
        <h3>Histogram</h3>
      </Col>
      <Col xs="auto">
        <EndpointButton endpoint={endpoint} fullpath="acq/graph/refresh_data" value={true}>
          Refresh Graph
        </EndpointButton>
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
                <Histogram endpoint={endpoint} data_endpoint={data_endpoint} />
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

export { GraphPage }