import { EndpointDropdown, EndpointSlider, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./App";
import { Col, Container, Form, InputGroup, Row, Stack } from "react-bootstrap";
import { Histogram } from "./Histogram";

interface GraphPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}

const GraphPage = ({ endpoint }: GraphPageProps) => {

  const Controls = (
    <Stack direction="horizontal" gap={2}>
      <Form>
        <Form.Label>Channel</Form.Label>
        <EndpointSlider endpoint={endpoint} fullpath="acq/graph/channel" />
      </Form>
      <InputGroup>
        <InputGroup.Text>Signal</InputGroup.Text>
        <EndpointDropdown endpoint={endpoint} fullpath="acq/graph/signal" />
      </InputGroup>
    </Stack>
  )

  return (
    <Container>
      <Row>
        <Col>
          <TitleCard title={Controls}>
            <Row>
              <Col>
                <Histogram endpoint={endpoint} />
              </Col>
            </Row>
          </TitleCard>
        </Col>
      </Row>
    </Container>
  )
}

export { GraphPage }