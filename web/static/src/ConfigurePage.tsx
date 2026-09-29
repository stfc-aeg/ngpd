import { EndpointButton, EndpointCheckbox, EndpointDoubleSlider, EndpointDropdown, EndpointInput, EndpointSelect, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import type { MouseEventHandler } from "react";
import { useState } from "react";
import { Button, Col, Container, FloatingLabel, Form, InputGroup, Row, Stack, Tab, Tabs } from "react-bootstrap";
import type { EndpointParams } from "./types";
import { channels } from "./types";

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
      <Row>
        <Col>
          <TitleCard title="Device">
            <Row>
              <Col lg="2" md="6">
                <EndpointButton endpoint={endpoint} fullpath="device/connect" value={true} variant={endpoint.data?.device.connect ? "success" : "primary"}>
                  {endpoint.data?.device.connect ? "System Configured" : "Configure System"}
                </EndpointButton>
              </Col>
              <Col lg="4" md="6">
                <FloatingLabel label="Base Board IP Address">
                  <EndpointInput endpoint={endpoint} fullpath="device/base_ip" />
                </FloatingLabel>
              </Col>
              <Col lg="3" md="6">
                <FloatingLabel label="Cards in System">
                  <EndpointInput endpoint={endpoint} fullpath="device/num_cards" />
                </FloatingLabel>
              </Col>
              <Col lg="3" md="6">
                <FloatingLabel label="Dummy Simulation">
                  <EndpointSelect endpoint={endpoint} fullpath="device/dummy_system" />
                </FloatingLabel>
              </Col>
            </Row>
          </TitleCard>
        </Col>
      </Row>
      <TitleCard title="Configuration">
        <Row>
          <Col>
            <Tabs defaultActiveKey={0} onSelect={(k) => setChan(parseInt(k ?? "0") as typeof channels[0])}>
              {channels.map((chan) => (
                <Tab eventKey={chan} title={`Channel ${chan}`} style={{ marginTop: "5px" }}>
                  <Row>
                    <Col lg="6" md="12">
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
                          <Row>
                            <Col xs="4">
                              <FloatingLabel label="Filter Type">
                                <EndpointSelect endpoint={endpoint} fullpath={`config/filter/channel_${chan}/type`} />
                              </FloatingLabel>
                            </Col>
                            <Col>
                              <InputGroup>
                                {endpoint.data?.config.filter[`channel_${chan}`].type == "rectangle" &&
                                  <FloatingLabel label="Averaging Window Size">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`}
                                      min={1} max={101} />
                                  </FloatingLabel>
                                }
                                {endpoint.data?.config.filter[`channel_${chan}`].type == "gaussian" &&
                                  <FloatingLabel label="Sigma">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`}
                                      min={1} max={25} step={0.5} />

                                  </FloatingLabel>
                                }
                                {endpoint.data?.config.filter[`channel_${chan}`].type == "exponential" &&
                                  <FloatingLabel label="T Samples">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`}
                                      min={1} max={25} step={0.5} />
                                  </FloatingLabel>
                                }
                                {endpoint.data?.config.filter[`channel_${chan}`].type == "trapezoidal" &&
                                  <>
                                    <FloatingLabel label="Top Width">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`}
                                        min={0} max={100} />
                                    </FloatingLabel>
                                    <FloatingLabel label="Bottom Width">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_2`}
                                        min={0} max={100} />
                                    </FloatingLabel>
                                  </>
                                }
                              </InputGroup>
                            </Col>
                          </Row>
                        </TitleCard>
                        <TitleCard title={<ConfigCardHeader title="Tail Measurement" onClick={() => setAllConfig("tail_measure")} />}>
                          <Row>
                            <Col>
                              <Row>
                                <Col xs="5">
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
                              <Row className="py-2">
                                <Col>
                                  <FloatingLabel label="Fall Time Fraction">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/fall_time_frac`} />
                                  </FloatingLabel>
                                </Col>
                              </Row>
                            </Col>
                            <Col sm="auto" lg="12" xl="auto" style={{ alignContent: "center" }}>
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
                            <Col xs={4}>
                              <Stack gap={2}>
                                <FloatingLabel label="Div Cont">
                                  <EndpointSelect endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/div_cont`} />
                                </FloatingLabel>
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/use_fixed`} type="switch" label="Use Fixed" />
                              </Stack>
                            </Col>
                          </Row>
                        </TitleCard>
                        <TitleCard title={<ConfigCardHeader title="Differential Trigger" onClick={() => setAllConfig("trigger")} />}>
                          <Row>
                            <Col xl="3" lg="6" md="3" xs="6" className="pb-2">
                              <Stack gap={2}>
                                <FloatingLabel label="Threshold">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/threshold`} />
                                </FloatingLabel>
                                <FloatingLabel label="Separation">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/separation`} />
                                </FloatingLabel>
                              </Stack>
                            </Col>
                            <Col className="pb-2">
                              <Stack gap={2}>
                                <FloatingLabel label="Data Delay">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/data_delay`} />
                                </FloatingLabel>
                                <FloatingLabel label="Trigger Delay">
                                  <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/trig_delay`} />
                                </FloatingLabel>
                              </Stack>
                            </Col>
                            <Col xl="6" lg="12" md="6" className="pb-2">
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
                          <Col lg="9" md="12">
                            <Row>
                              <Col lg="6" md="12">
                                <EndpointDoubleSlider endpoint={endpoint} title="Pulse height"
                                  fullpath={[
                                    `config/discrimination/channel_${chan}/height_min`,
                                    `config/discrimination/channel_${chan}/height_max`
                                  ]} />
                              </Col>
                              <Col>
                                <EndpointDoubleSlider endpoint={endpoint} title="Fall Time"
                                  fullpath={[
                                    `config/discrimination/channel_${chan}/min_fall`,
                                    `config/discrimination/channel_${chan}/max_fall`
                                  ]} />
                              </Col>
                            </Row>
                            <Row>
                              <Col lg="8" md="12" className="mb-2">
                                <InputGroup>
                                  <InputGroup.Text>Threshold Calculations</InputGroup.Text>
                                  <FloatingLabel label="Intercept">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_c`} />
                                  </FloatingLabel>
                                  <FloatingLabel label="Gradient">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_m`} />
                                  </FloatingLabel>
                                </InputGroup>
                              </Col>
                              <Col className="mb-2">
                                <InputGroup>
                                  <InputGroup.Text>Pulse Count</InputGroup.Text>
                                  <FloatingLabel label="Minimum">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/min_count`} />
                                  </FloatingLabel>
                                </InputGroup>
                              </Col>
                            </Row>
                          </Col>
                          <Col lg="3" md="12" style={{ alignContent: "center" }}>
                            <Row>
                              <Col lg="12" md="auto">
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/adaptive`} type="switch" label="Adaptive Tail Sum" />
                              </Col>
                              <Col lg="12" md="auto">
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_tail_sum`} type="switch" label="Measure Tail Sum" />
                              </Col>
                              <Col lg="12" md="auto">
                                <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_fall_time`} type="switch" label="Measure Fall Time" />
                              </Col>
                            </Row>
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


export { ConfigPage };
