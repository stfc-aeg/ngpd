import { EndpointButton, EndpointCheckbox, EndpointDropdown, EndpointInput, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import type { EndpointParams } from "./types";
import { channels } from "./types";
import { Container, Row, Col, Stack, Card, InputGroup, FloatingLabel, Collapse, Form, Tabs, Tab, Button } from "react-bootstrap";
import { useState } from "react";
import type { MouseEventHandler, PropsWithChildren, ReactNode } from "react";

interface ConfigPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}


const ConfigCardHeader = ({ title, onClick }: { title: string, onClick: MouseEventHandler }) => {

  return (
    <Row>
      <Col style={{ alignContent: "center" }}>
        {title}
      </Col>
      <Col xs="auto">
        <Button size="sm" onClick={onClick}>Apply to All</Button>
      </Col>
    </Row>
  )
}

const ConfigPage = ({ endpoint }: ConfigPageProps) => {

  const [chan, setChan] = useState<typeof channels[0]>(0);

  const setAllConfig = (configPath: keyof EndpointParams["config"]) => {
    if (configPath == "playback" || configPath == "histogram") {
      return;
    }

    console.log(`Getting ${configPath} config of channel ${chan}`);
    const current_chan_settings = endpoint.data?.config[configPath][`channel_${chan}`];
    if (current_chan_settings) {
      channels.forEach((channel) =>
        endpoint.put(current_chan_settings, `config/${configPath}/channel_${channel}`)
      )
    }

  }

  return (
    <Container fluid="xxl">
      <TitleCard title="Configuration">
        <Row>
          <Col>
            <TitleCard title="Device">
              <Row>
                <Col xs="auto">
                  <EndpointButton endpoint={endpoint} fullpath="device/connect" value={true} variant={endpoint.data?.device.connect ? "success" : "primary"}>
                    {endpoint.data?.device.connect ? "System Configured" : "Configure System"}
                  </EndpointButton>
                </Col>
                <Col>
                  <InputGroup>
                    <InputGroup.Text>Base Board IP Address</InputGroup.Text>
                    <EndpointInput endpoint={endpoint} fullpath="device/base_ip" />
                  </InputGroup>
                </Col>
                <Col xs="auto">
                  <InputGroup>
                    <InputGroup.Text>Cards in System</InputGroup.Text>
                    <EndpointInput endpoint={endpoint} fullpath="device/num_cards" />
                  </InputGroup>
                </Col>
                <Col xs="auto">
                  <InputGroup>
                    <InputGroup.Text>Dummy Simulation</InputGroup.Text>
                    <EndpointDropdown endpoint={endpoint} fullpath="device/dummy_system" />
                  </InputGroup>
                </Col>
              </Row>
            </TitleCard>
          </Col>
        </Row>
        <Row style={{ marginTop: "16px" }}>
          <hr />
        </Row>
        <Row>
          <Col>
            <Tabs defaultActiveKey={0} onSelect={(k) => setChan(parseInt(k ?? "0") as typeof channels[0])} transition={false}>
              {channels.map((chan) => (
                <Tab eventKey={chan} title={`Channel ${chan}`} style={{ marginTop: "5px" }}>
                  <Row>
                    <Col>
                      <Stack>
                        <TitleCard title={<ConfigCardHeader title="Analog Input" onClick={() => setAllConfig("analog")} />}>
                          <Row>
                            <Col>
                              <FloatingLabel label="Gain">
                                <EndpointInput endpoint={endpoint} fullpath={`config/analog/channel_${chan}/gain`} />
                              </FloatingLabel>
                            </Col>
                            <Col>
                              <FloatingLabel label="Offset">
                                <EndpointInput endpoint={endpoint} fullpath={`config/analog/channel_${chan}/offset`} />
                              </FloatingLabel>
                            </Col>
                          </Row>
                        </TitleCard>
                        <TitleCard title={<ConfigCardHeader title="Filter" onClick={() => setAllConfig("filter")} />}>
                          <InputGroup>
                            <InputGroup.Text>Type</InputGroup.Text>
                            <EndpointDropdown endpoint={endpoint} fullpath={`config/filter/channel_${chan}/type`} />
                            {endpoint.data?.config.filter[`channel_${chan}`].type == "rectangle" &&
                              <FloatingLabel label="Averaging Window Size">
                                <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`} />
                              </FloatingLabel>
                            }
                            {endpoint.data?.config.filter[`channel_${chan}`].type == "gaussian" &&
                              <FloatingLabel label="Signma">
                                <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`} />
                              </FloatingLabel>
                            }
                            {endpoint.data?.config.filter[`channel_${chan}`].type == "exponential" &&
                              <FloatingLabel label="T Samples">
                                <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`} />
                              </FloatingLabel>
                            }
                            {endpoint.data?.config.filter[`channel_${chan}`].type == "trapezoidal" &&
                              <>
                                <FloatingLabel label="Top Width">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`} />
                                </FloatingLabel>
                                <FloatingLabel label="Bottom Width">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_2`} />
                                </FloatingLabel>
                              </>
                            }
                          </InputGroup>
                        </TitleCard>
                        <TitleCard title={<ConfigCardHeader title="Tail Measurement" onClick={() => setAllConfig("tail_measure")} />}>
                          <Row>
                            <Col>
                              <Stack gap={2}>
                                <Row>
                                  <Col xs={5}>
                                    <FloatingLabel label="Delay">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/delay`} />
                                    </FloatingLabel>
                                  </Col>
                                  <Col>
                                    <FloatingLabel label="Sample Number">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/num_sample`} />
                                    </FloatingLabel>
                                  </Col>
                                </Row>
                                <Col>
                                  <FloatingLabel label="Fall Time Fraction">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/fall_time_frac`} />
                                  </FloatingLabel>
                                </Col>
                              </Stack>
                            </Col>
                            <Col xs="auto" style={{ alignContent: "center" }}>
                              <Form>
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_tail_subtract`}
                                  type="switch" label="Enable Tail Subtract" />
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_subtract_test`}
                                  type="switch" label="Enable Tail Subtract Test" />
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_subtract_neutron`}
                                  type="switch" label="Enable Neutron Subtract" />
                              </Form>
                            </Col>
                          </Row>
                        </TitleCard>
                      </Stack>
                    </Col>
                    <Col>
                      <Stack>
                        <TitleCard title={<ConfigCardHeader title="Baseline Subtraction" onClick={() => setAllConfig("base_sub")} />}>
                          <Row>
                            <Col>
                              <Stack gap={2}>
                                <FloatingLabel label="Fixed">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/fixed`} />
                                </FloatingLabel>
                                <FloatingLabel label="Error Limit">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/error_limit`} />
                                </FloatingLabel>
                              </Stack>
                            </Col>
                            <Col xs={4} style={{ alignContent: "center" }}>
                              <Stack gap={2}>
                                <InputGroup>
                                  <InputGroup.Text>Div Cont</InputGroup.Text>
                                  <EndpointDropdown endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/div_cont`} />
                                </InputGroup>
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/use_fixed`} type="switch" label="Use Fixed" />
                              </Stack>
                            </Col>
                          </Row>
                        </TitleCard>
                        <TitleCard title={<ConfigCardHeader title="Differential Trigger" onClick={() => setAllConfig("trigger")} />}>
                          <Row>
                            <Col md={3}>
                              <Stack gap={2}>
                                <FloatingLabel label="Threshold">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/threshold`} />
                                </FloatingLabel>
                                <FloatingLabel label="Separation">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/separation`} />
                                </FloatingLabel>
                              </Stack>
                            </Col>
                            <Col>
                              <Stack gap={2}>
                                <FloatingLabel label="Data Delay">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/data_delay`} />
                                </FloatingLabel>
                                <FloatingLabel label="Trigger Delay">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/trig_delay`} />
                                </FloatingLabel>
                              </Stack>
                            </Col>
                            <Col md={6}>
                              <Stack gap={2}>
                                <InputGroup>
                                  <InputGroup.Text>Signal A</InputGroup.Text>
                                  <FloatingLabel label="Delay">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/delay_a`} />
                                  </FloatingLabel>
                                  <FloatingLabel label="Width">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/width_a`} />
                                  </FloatingLabel>
                                </InputGroup>
                                <InputGroup>
                                  <InputGroup.Text>Signal B</InputGroup.Text>
                                  <FloatingLabel label="Delay">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/delay_b`} />
                                  </FloatingLabel>
                                  <FloatingLabel label="Width">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/width_b`} />
                                  </FloatingLabel>
                                </InputGroup>
                              </Stack>
                            </Col>
                          </Row>
                        </TitleCard>
                      </Stack>
                    </Col>
                  </Row>
                  <Row>
                    <Col>
                      <TitleCard title={<ConfigCardHeader title="Neutron/Gamma Discrimination" onClick={() => setAllConfig("discrimination")} />}>
                        <Row>
                          <Col>
                            <Stack gap={2}>
                              <InputGroup>
                                <InputGroup.Text>Pulse Height</InputGroup.Text>
                                <FloatingLabel label="Minimum">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/height_min`} />
                                </FloatingLabel>
                                <FloatingLabel label="Maximum">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/height_max`} />
                                </FloatingLabel>
                              </InputGroup>
                              <InputGroup>
                                <InputGroup.Text>Threshold Calculations</InputGroup.Text>
                                <FloatingLabel label="C Parameter">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_c`} />
                                </FloatingLabel>
                                <FloatingLabel label="M Parameter">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_m`} />
                                </FloatingLabel>
                              </InputGroup>
                            </Stack>
                          </Col>
                          <Col>
                            <Stack gap={2}>
                              <InputGroup>
                                <InputGroup.Text>Fall Time</InputGroup.Text>
                                <FloatingLabel label="Minimum">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/min_fall`} />
                                </FloatingLabel>
                                <FloatingLabel label="Maximum">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/max_fall`} />
                                </FloatingLabel>
                              </InputGroup>
                              <InputGroup>
                                <InputGroup.Text>Pulse Count</InputGroup.Text>
                                <FloatingLabel label="Minimum">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/min_count`} />
                                </FloatingLabel>
                              </InputGroup>
                            </Stack>
                          </Col>
                          <Col xl="auto" md={12} style={{ alignContent: "center" }}>
                            <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/adaptive`} type="switch" label="Adaptive Tail Sum" />
                            <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_tail_sum`} type="switch" label="Measure Tail Sum" />
                            <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_fall_time`} type="switch" label="Measure Fall Time" />
                          </Col>
                        </Row>
                      </TitleCard>
                    </Col>
                  </Row>
                </Tab>
              ))}
            </Tabs>
          </Col>
        </Row>

      </TitleCard>
    </Container >
  )
}


export { ConfigPage }