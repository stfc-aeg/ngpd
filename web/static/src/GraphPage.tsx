import { EndpointButton, EndpointCheckbox, EndpointDropdown, EndpointInput, EndpointSelect, TitleCard, useAdapterEndpoint, type AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./types";
import { Badge, Button, ButtonGroup, Col, Container, DropdownItem, FloatingLabel, Form, InputGroup, ProgressBar, Row, Stack } from "react-bootstrap";
import { Histogram } from "./Histogram";

interface GraphPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
  data_endpoint: AdapterEndpoint<{ value: EndpointParams["acq"]["graph"]["hist"] }>;
}

const GraphPage = ({ endpoint, data_endpoint }: GraphPageProps) => {

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
            <Stack gap={2}>
              <Histogram endpoint={endpoint} data_endpoint={data_endpoint} />

            </Stack>
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
              <ProgressBar label={endpoint.data?.acq.state.status} now={endpoint.data?.acq.state.current} max={endpoint.data?.acq.state.total}/>
              </Col>
            </Row>
          </TitleCard >
        </Col >
      </Row >
    </Container >
  )
}

export { GraphPage }