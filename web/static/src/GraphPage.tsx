import { EndpointButton, EndpointCheckbox, EndpointDropdown, EndpointInput, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./App";
import { ButtonGroup, Col, Container, DropdownItem, FloatingLabel, Form, InputGroup, Row, Stack } from "react-bootstrap";
import { Histogram } from "./Histogram";

interface GraphPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}

const GraphPage = ({ endpoint }: GraphPageProps) => {

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
    <Container fluid="xxl">
      <Row>
        <Col>
          <TitleCard title={Controls}>
            <Stack gap={2}>
              <Histogram endpoint={endpoint} />

            </Stack>
          </TitleCard>
        </Col>
      </Row>
      <Row>
        <Col>
          <TitleCard title="Histogram Controls">
            <Row>
              <Col>
                <Form.Label column>Pulse Height</Form.Label>
                <InputGroup>
                  <FloatingLabel label="Bit Shift">
                    <EndpointInput endpoint={endpoint} fullpath="config/histogram/shift_height" size="sm" />
                  </FloatingLabel>
                  <EndpointDropdown endpoint={endpoint} fullpath="config/histogram/num_bins_height" title={`Num Bins: ${endpoint.data?.config.histogram.num_bins_height ?? 0}`} />
                </InputGroup>
              </Col>
              <Col>
                <Row>
                  <Form.Label column>Tail Sum</Form.Label>
                  <Col xs="auto" style={{ alignContent: "center" }}><EndpointCheckbox endpoint={endpoint} fullpath="config/histogram/separate_ngp" type="switch" label="Separate NGP" /></Col>
                </Row>
                <InputGroup>
                  <FloatingLabel label="Bit Shift">
                    <EndpointInput endpoint={endpoint} fullpath="config/histogram/shift_tailsum" size="sm" />
                  </FloatingLabel>
                  <EndpointDropdown endpoint={endpoint} fullpath="config/histogram/num_bins_tailsum" title={`Num Bins: ${endpoint.data?.config.histogram.num_bins_tailsum ?? 0}`} />
                </InputGroup>
              </Col>
            </Row>
          </TitleCard>
        </Col>
        <Col>
          <TitleCard title="Run Controls">
            <Row>
              <Col>
                <Stack gap={2}>
                  <InputGroup>
                    <InputGroup.Text>Num Cycles</InputGroup.Text>
                    <EndpointInput endpoint={endpoint} fullpath="acq/num_cycles" />
                  </InputGroup>
                  <Form>
                    <EndpointCheckbox endpoint={endpoint} fullpath="acq/scope_setup" type="switch" label="Setup Scope" />
                    <EndpointCheckbox endpoint={endpoint} fullpath="acq/scope_run" type="switch" label="Run Scope" />
                  </Form>
                </Stack>
              </Col>
              <Col>
                <Stack gap={2}>
                  <InputGroup>
                    <InputGroup.Text>Frame Length (s)</InputGroup.Text>
                    <EndpointInput endpoint={endpoint} fullpath="acq/frame_length" />
                  </InputGroup>
                  <ButtonGroup>
                    <EndpointButton endpoint={endpoint} fullpath="acq/run" value={true} disabled={endpoint.data?.acq.state.status == "running"}>
                      Start
                    </EndpointButton>
                    <EndpointButton endpoint={endpoint} fullpath="acq/run" value={true} disabled={endpoint.data?.acq.state.status != "running"} variant="danger">
                      Stop
                    </EndpointButton>
                  </ButtonGroup>
                </Stack>
              </Col>
            </Row>
          </TitleCard>
        </Col>
      </Row>
    </Container>
  )
}

export { GraphPage }