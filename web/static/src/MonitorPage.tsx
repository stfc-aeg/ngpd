import { EndpointInput, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import { Col, Container, FloatingLabel, Form, InputGroup, ListGroup, Row } from "react-bootstrap";
import { CheckCircle, ExclamationCircle } from 'react-bootstrap-icons';
import type { EndpointParams } from "./types";

import styles from './style.module.css';

interface MonitorPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}

interface ThemometerProps {
  current: number;
  max?: number;
  label?: string;

}

const Themometer = ({ current, max = 1000, label }: ThemometerProps) => {

  const style_prefix = max < 0 ? "thermometerSuccess" : current < max * 0.9 ? "thermometerSuccess" : current < max ? "thermometerWarn" : "thermometerError";
  const Icon = style_prefix == "thermometerError" ? <ExclamationCircle /> : <CheckCircle />;

  return (
    <>
      <InputGroup.Text className={styles[style_prefix]}>{Icon}</InputGroup.Text>
      <FloatingLabel label={label} className={styles[style_prefix]} >
        <Form.Control value={`${current.toFixed(2)}°c`} readOnly />
      </FloatingLabel>
    </>
  )
}

const MonitorPage = ({ endpoint }: MonitorPageProps) => {

  const voltListHeight = "calc(100vh - 20rem)";
  return (
    <Container fluid="xxl">
      <Row>
        <Col>
          <TitleCard title="Temperatures">
            <Row>
              <Col md="4" sm="12">
                <InputGroup>
                  <Themometer label="ADC" current={endpoint.data?.monitor.adc.temperature ?? 0} max={endpoint.data?.monitor.adc.trip_temp} />
                  <FloatingLabel label="Over Temp Limit">
                    <EndpointInput endpoint={endpoint} fullpath="monitor/adc/trip_temp" />
                  </FloatingLabel>
                </InputGroup>
              </Col>
              <Col md="4" sm="12">
                <InputGroup>
                  <Themometer label="PreAmp" current={endpoint.data?.monitor.preamp.temperature ?? 0} max={endpoint.data?.monitor.preamp.trip_temp} />
                  <FloatingLabel label="Over Temp Limit">
                    <EndpointInput endpoint={endpoint} fullpath="monitor/preamp/trip_temp" disabled={(endpoint.data?.monitor.preamp.trip_temp ?? -1) < 0} />
                  </FloatingLabel>
                </InputGroup>
              </Col>
              <Col md="4" sm="12">
                <InputGroup>
                  <Themometer label="FPGA" current={endpoint.data?.monitor.fpga.temperature ?? 0} max={-1} />
                  <FloatingLabel label="Over Temp Limit (Not Implemented)">
                    <Form.Control disabled value={-1} />
                  </FloatingLabel>
                </InputGroup>
              </Col>
            </Row>
          </TitleCard>
        </Col>
      </Row>
      <Row>
        <Col md="6" sm="12">
          <TitleCard title="ADC Voltages">
            <ListGroup style={{ maxHeight: voltListHeight, overflowY: "auto", scrollSnapType: "y mandatory" }}>
              {Object.entries(endpoint.data?.monitor.adc.voltages ?? {}).map(
                ([key, val]) => (
                  <ListGroup.Item>
                    <Row>
                      <Col>{key}</Col>
                      <Col xs="auto">{val.toFixed(3)}V</Col>
                    </Row>
                  </ListGroup.Item>
                )
              )}
            </ListGroup>
          </TitleCard>
        </Col>
        <Col>
          <TitleCard title="FPGA Voltages">
            <ListGroup style={{ maxHeight: voltListHeight, overflowY: "auto", scrollSnapType: "y mandatory" }}>
              {Object.entries(endpoint.data?.monitor.fpga.voltages ?? {}).map(
                ([key, val]) => (
                  <ListGroup.Item>
                    <Row>
                      <Col>{key}</Col>
                      <Col xs="auto">{val.toFixed(3)}V</Col>
                    </Row>
                  </ListGroup.Item>
                )
              )}
            </ListGroup>
          </TitleCard>
        </Col>

      </Row>
    </Container>
  )
}

export { MonitorPage };
