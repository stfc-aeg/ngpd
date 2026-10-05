import { EndpointButton, EndpointCheckbox, EndpointDoubleSlider, EndpointInput, EndpointSelect, TitleCard, type AdapterEndpoint } from "@dssg/odin-react";
import type { SubmitEventHandler } from "react";
import { useRef } from "react";
import { Button, Col, Container, FloatingLabel, Form, InputGroup, Row, Stack, Tab, Tabs } from "react-bootstrap";
import type { EndpointParams } from "./types";
import { channels } from "./types";

interface ConfigPageProps {
  endpoint: AdapterEndpoint<EndpointParams>;
}

interface ControlCardProps {
  endpoint: AdapterEndpoint<EndpointParams>;
  chan: typeof channels[0];

}

interface GenericControlCardProps extends ControlCardProps {
  title: string;
  configPath: keyof EndpointParams["config"];
  children?: React.ReactNode;
}

const isSelectElement = (element: HTMLInputElement | HTMLSelectElement): element is HTMLSelectElement => {
  return element.tagName === "SELECT"
}


const ConfigControls = ({ endpoint, chan, title, configPath, children }: GenericControlCardProps) => {

  const FormRef = useRef<HTMLFormElement>(null);
  const SubmitHandler: SubmitEventHandler<HTMLFormElement> = (event) => {
    console.log(event.nativeEvent);
    event.preventDefault();
    if (configPath == "playback" || configPath == "histogram") {
      return;
    }
    const form = event.target;
    console.log(form.elements);
    if (form.checkValidity()) {
      const data: Record<string, number | boolean | string> = endpoint.data?.config[configPath][`channel_${chan}`] ?? {};

      const elements = Array.from(form.elements) as (HTMLInputElement | HTMLSelectElement)[];

      elements.filter(element => element.type !== "submit").forEach(element => {
        if (isSelectElement(element)) {
          data[element.name] = isNaN(Number(element.value)) ? element.value : Number(element.value);
        } else {
          switch (element.type) {
            case "number":
              data[element.name] = element.valueAsNumber;
              element.dispatchEvent(new KeyboardEvent("keydown", { key: "Enter", bubbles: true }));
              break;
            case "checkbox":
              data[element.name] = element.checked;
              break;
            case "range":
              //Im using Double slider stuff Im not 100% how to handle this
              console.log("Dunno how to handle SLider yet");
              console.log(element);
              break;
            default:
              data[element.name] = element.value;
              break;
          }
        }
      })
      console.log(data);

      channels.forEach((channel) => endpoint.put(data, `config/${configPath}/channel_${channel}`));
    }
  }

  const FormId = `${title}_${chan}`;

  const Header = (
    <Row>
      <Col style={{ alignContent: "center" }}>
        {title}
      </Col>
      <Col xs="auto">
        <Button size="sm" onClick={() => FormRef.current?.requestSubmit()}>
          Set All Channels
        </Button>
      </Col>
    </Row>
  )

  return (
    <TitleCard title={Header}>
      <Form ref={FormRef} id={FormId} onSubmit={SubmitHandler} noValidate>
        {children}
      </Form>
    </TitleCard>
  )
}


const ConfigPage = ({ endpoint }: ConfigPageProps) => {

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
                <Tabs defaultActiveKey={0} unmountOnExit >
                  {channels.map((chan) => (
                    <Tab eventKey={chan} key={`${chan}_tab`} title={`Channel ${chan}`} style={{ marginTop: "5px" }}>
                      <Row>
                        <Col lg="6" md="12">
                          <Stack>
                            <ConfigControls endpoint={endpoint} chan={chan} title="Analog Input" configPath="analog">
                              <Row>
                                <Col>
                                  <FloatingLabel label="Attenuation">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/analog/channel_${chan}/gain`} name="gain" />
                                  </FloatingLabel>
                                </Col>
                                <Col>
                                  <FloatingLabel label="Offset">
                                    <EndpointInput endpoint={endpoint} fullpath={`config/analog/channel_${chan}/offset`} name="offset" />
                                  </FloatingLabel>
                                </Col>
                              </Row>
                            </ConfigControls>
                            <ConfigControls endpoint={endpoint} chan={chan} title="Filter" configPath="filter">
                              <Row>
                                <Col xs="4">
                                  <FloatingLabel label="Filter Type">
                                    <EndpointSelect endpoint={endpoint} fullpath={`config/filter/channel_${chan}/type`} name="type" defaultValue="Unknown" />
                                  </FloatingLabel>
                                  <button type="submit" disabled hidden id="secretHidenButtonToStopDefaultSubmit" />
                                </Col>
                                <Col>
                                  <InputGroup>
                                    {endpoint.data?.config.filter[`channel_${chan}`].type == "rectangle" &&
                                      <FloatingLabel label="Averaging Window Size">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`} name="arg_1"
                                          min={1} max={101} />
                                      </FloatingLabel>
                                    }
                                    {endpoint.data?.config.filter[`channel_${chan}`].type == "gaussian" &&
                                      <FloatingLabel label="Sigma">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`} name="arg_float"
                                          min={1} max={25} step={0.5} />

                                      </FloatingLabel>
                                    }
                                    {endpoint.data?.config.filter[`channel_${chan}`].type == "exponential" &&
                                      <FloatingLabel label="T Samples">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_float`} name="arg_float"
                                          min={1} max={25} step={0.5} />
                                      </FloatingLabel>
                                    }
                                    {endpoint.data?.config.filter[`channel_${chan}`].type == "trapezoidal" &&
                                      <>
                                        <FloatingLabel label="Top Width">
                                          <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_1`} name="arg_1"
                                            min={0} max={100} />
                                        </FloatingLabel>
                                        <FloatingLabel label="Bottom Width">
                                          <EndpointInput endpoint={endpoint} fullpath={`config/filter/channel_${chan}/arg_2`} name="arg_2"
                                            min={0} max={100} />
                                        </FloatingLabel>
                                      </>
                                    }
                                  </InputGroup>
                                </Col>
                              </Row>
                            </ConfigControls>
                            <ConfigControls endpoint={endpoint} chan={chan} title="Tail Measurement" configPath="tail_measure">
                              <Row>
                                <Col>
                                  <Row>
                                    <Col xs="5">
                                      <FloatingLabel label="Delay">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/delay`} name="delay" />
                                      </FloatingLabel>
                                    </Col>
                                    <Col>
                                      <FloatingLabel label="Sample Number">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/num_sample`} name="num_sample" />
                                      </FloatingLabel>
                                    </Col>
                                  </Row>
                                  <Row className="py-2">
                                    <Col>
                                      <FloatingLabel label="Fall Time Fraction">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/fall_time_frac`} name="fall_time_frac" step="any" />
                                      </FloatingLabel>
                                    </Col>
                                  </Row>
                                </Col>
                                <Col sm="auto" lg="12" xl="auto" style={{ alignContent: "center" }}>
                                  <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_tail_subtract`} name="enable_tail_subtract"
                                    type="switch" label="Enable Tail Subtract" />
                                  <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_subtract_test`} name="enable_subtract_test"
                                    type="switch" label="Enable Tail Subtract Test" />
                                  <EndpointCheckbox endpoint={endpoint} fullpath={`config/tail_measure/channel_${chan}/enable_subtract_neutron`} name="enable_subtract_neutron"
                                    type="switch" label="Enable Neutron Subtract" />
                                </Col>
                              </Row>
                            </ConfigControls>
                          </Stack>
                        </Col>
                        <Col>
                          <Stack>
                            <ConfigControls endpoint={endpoint} chan={chan} title="Baseline Subtraction" configPath="base_sub">
                              <Row>
                                <Col>
                                  <Stack gap={2}>
                                    <FloatingLabel label="Fixed">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/fixed`} name="fixed" />
                                    </FloatingLabel>
                                    <FloatingLabel label="Error Limit">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/error_limit`} name="error_limit" />
                                    </FloatingLabel>
                                  </Stack>
                                </Col>
                                <Col xs={4}>
                                  <Stack gap={2}>
                                    <FloatingLabel label="Div Cont">
                                      <EndpointSelect endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/div_cont`} name="div_cont" />
                                    </FloatingLabel>
                                    <EndpointCheckbox endpoint={endpoint} fullpath={`config/base_sub/channel_${chan}/use_fixed`} type="switch" label="Use Fixed" name="use_fixed" />
                                  </Stack>
                                </Col>
                              </Row>
                            </ConfigControls>
                            <ConfigControls endpoint={endpoint} chan={chan} title="Trigger" configPath="trigger">
                              <Row>
                                <Col xl="3" lg="6" md="3" xs="6" className="pb-2">
                                  <Stack gap={2}>
                                    <FloatingLabel label="Threshold">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/threshold`} name="threshold" />
                                    </FloatingLabel>
                                    <FloatingLabel label="Separation">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/separation`} name="separation" />
                                    </FloatingLabel>
                                  </Stack>
                                </Col>
                                <Col className="pb-2">
                                  <Stack gap={2}>
                                    <FloatingLabel label="Data Delay">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/data_delay`} name="data_delay" />
                                    </FloatingLabel>
                                    <FloatingLabel label="Trigger Delay">
                                      <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/trig_delay`} name="trig_delay" />
                                    </FloatingLabel>
                                  </Stack>
                                </Col>
                                <Col xl="6" lg="12" md="6" className="pb-2">
                                  <Stack gap={2}>
                                    <InputGroup>
                                      <InputGroup.Text>Signal A</InputGroup.Text>
                                      <FloatingLabel label="Delay">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/delay_a`} name="delay_a" />
                                      </FloatingLabel>
                                      <FloatingLabel label="Width">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/width_a`} name="width_a" />
                                      </FloatingLabel>
                                    </InputGroup>
                                    <InputGroup>
                                      <InputGroup.Text>Signal B</InputGroup.Text>
                                      <FloatingLabel label="Delay">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/delay_b`} name="delay_b" />
                                      </FloatingLabel>
                                      <FloatingLabel label="Width">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/trigger/channel_${chan}/width_b`} name="width_b" />
                                      </FloatingLabel>
                                    </InputGroup>
                                  </Stack>
                                </Col>
                              </Row>
                            </ConfigControls>
                          </Stack>
                        </Col>
                      </Row>
                      <Row>
                        <Col>
                          <ConfigControls endpoint={endpoint} chan={chan} title="Neutron/Gamma Discrimination" configPath="discrimination">
                            <Row>
                              <Col lg="9" md="12">
                                <Row>
                                  <Col lg="6" md="12" className="mb-2">
                                    <InputGroup>
                                      <InputGroup.Text>Pulse Height</InputGroup.Text>
                                      <FloatingLabel label="Minimum">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/height_min`} name="height_min"/>
                                      </FloatingLabel>
                                      <FloatingLabel label="Maximum">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/height_max`} name="height_max"/>
                                      </FloatingLabel>
                                    </InputGroup>
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
                                        <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_c`} name="threshold_c" />
                                      </FloatingLabel>
                                      <FloatingLabel label="Gradient">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/threshold_m`} name="threshold_m" />
                                      </FloatingLabel>
                                    </InputGroup>
                                  </Col>
                                  <Col className="mb-2">
                                    <InputGroup>
                                      <InputGroup.Text>Pulse Count</InputGroup.Text>
                                      <FloatingLabel label="Minimum">
                                        <EndpointInput endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/min_count`} name="min_count" />
                                      </FloatingLabel>
                                    </InputGroup>
                                  </Col>
                                </Row>
                              </Col>
                              <Col lg="3" md="12" style={{ alignContent: "center" }}>
                                <Row>
                                  <Col lg="12" md="auto">
                                    <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/adaptive`} type="switch" label="Adaptive Tail Sum" name="adaptive" />
                                  </Col>
                                  <Col lg="12" md="auto">
                                    <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_tail_sum`} type="switch" label="Measure Tail Sum" name="enable_tail_sum" />
                                  </Col>
                                  <Col lg="12" md="auto">
                                    <EndpointCheckbox endpoint={endpoint} fullpath={`config/discrimination/channel_${chan}/enable_fall_time`} type="switch" label="Measure Fall Time" name="enable_fall_time" />
                                  </Col>
                                </Row>
                              </Col>
                            </Row>
                          </ConfigControls>
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
