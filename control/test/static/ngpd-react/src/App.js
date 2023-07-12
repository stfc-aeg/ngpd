
import React, { useEffect, useMemo, useState } from 'react';

import 'odin-react/dist/index.css'

import 'bootstrap/dist/css/bootstrap.min.css';

import { DropdownSelector, OdinApp, ScopeCanvas, TitleCard, ToggleSwitch } from 'odin-react';
import { WithEndpoint, useAdapterEndpoint } from 'odin-react';

import Row from 'react-bootstrap/Row';

// import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import Container from 'react-bootstrap/Container';
import Button from 'react-bootstrap/Button';
import Form from 'react-bootstrap/Form';
import InputGroup from 'react-bootstrap/InputGroup';
import Stack from 'react-bootstrap/Stack';
import Dropdown from 'react-bootstrap/Dropdown'
import Alert from 'react-bootstrap/Alert';


const EndpointButton = WithEndpoint(Button);
const EndpointInput = WithEndpoint(Form.Control);
const EndpointDropdown = WithEndpoint(DropdownSelector);
const EndpointToggle = WithEndpoint(ToggleSwitch);

const App = () => {

  const ngpdEndpoint = useAdapterEndpoint("ngpd", process.env.REACT_APP_ENDPOINT_URL);
  const dataEndpoint = useAdapterEndpoint("ngpd/data", process.env.REACT_APP_ENDPOINT_URL);

  const setup_data = ngpdEndpoint.data?.setup ? ngpdEndpoint.data.setup : {};
  const filter_data = ngpdEndpoint.data?.filter ? ngpdEndpoint.data.filter : {};
  const trigger_data = ngpdEndpoint.data?.trigger ? ngpdEndpoint.data.trigger.settings : {};
  const base_sub_data = ngpdEndpoint.data?.base_sub ? ngpdEndpoint.data.base_sub : {settings: {}};
  const measure_data = ngpdEndpoint.data?.measure ? ngpdEndpoint.data.measure.settings : {};
  const adc_data = ngpdEndpoint.data?.adc ? ngpdEndpoint.data.adc : {};
  const scope_options_data = ngpdEndpoint.data?.scope_options || {};

  const base_sub_type = ngpdEndpoint.data?.base_sub ? base_sub_data.div_cont_options[base_sub_data.settings.div_cont] : "Unknown";
  const scope_src = ngpdEndpoint.data?.scope_options ? ngpdEndpoint.data.scope_src_options[scope_options_data.scope_src] : "Unknown";

  const stack_gap = 2

  const [data_points, changeDataPoints] = useState(100);
  const [save_file, changeSaveFile] = useState("");


  const onChangeDataPoints = (event) => {
    console.log(event)
    changeDataPoints(+event.target.value);
  }

  const onChangeFileName = (event) => {
    changeSaveFile(event.target.value);
  }
  

  const raw_data =  useMemo(() => [{label: "Raw Scope Data",
                    x: ngpdEndpoint.data.data ? Array.from(ngpdEndpoint.data.data.raw_data, (_, i) => i): [0,1],
                    y: ngpdEndpoint.data.data ? ngpdEndpoint.data.data.raw_data: [0,1]
                   }], [ngpdEndpoint.data.data?.raw_data]);

  return (
    <OdinApp title="Neutron Gamma Pulse Discriminator"
             navLinks={["Configure", "Scope Data"]}>
      <Container>
      <Row>
      <Col md={4}>
        <TitleCard title="Setup">
          <Stack gap={stack_gap}>
            <Stack direction='horizontal' gap={stack_gap}>
              <Alert variant={setup_data.is_setup ? "success" : "danger"}>Is Connected: {setup_data.is_setup ? "Yes" : "No"}</Alert>
            </Stack>
            <Stack direction='horizontal' gap={stack_gap}>
              <InputGroup>
                <InputGroup.Text>Path</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="setup/path" readOnly disabled></EndpointInput>
              </InputGroup>
              <InputGroup>
                <InputGroup.Text>Channel</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="setup/channel" readOnly disabled></EndpointInput>
              </InputGroup>
            </Stack>
              <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="setup/setup_adq" value={true}>Setup ADQ</EndpointButton>
          </Stack>
        </TitleCard>
      </Col>
      <Col>
      <TitleCard title="Filter">
        <Stack gap={stack_gap}>
          <Col>
            
          </Col>
          {/* </Stack> */}
        <Stack direction="horizontal" gap={stack_gap}>
          <EndpointDropdown endpoint={ngpdEndpoint} event_type="select" fullpath="filter/type" buttonText={filter_data.type}>
              {filter_data.type_options ? filter_data.type_options.map(
                (selected_type) => (
                  <Dropdown.Item eventKey={selected_type} key={selected_type} active={filter_data.type === selected_type}>{selected_type}</Dropdown.Item>
                )) : <></>
              }
          </EndpointDropdown>
          <InputGroup>
            <InputGroup.Text>T Samples</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} fullpath="filter/tsamples" disabled={!(filter_data.type==="exp")} />
          </InputGroup>
          <InputGroup>
            <InputGroup.Text>Num Average</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="filter/num_ave" disabled={!(filter_data.type==="ave")}/>
          </InputGroup>
          <InputGroup>
            <InputGroup.Text>Sigma</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="filter/sigma" disabled={!(filter_data.type==="gaussian")}/>
          </InputGroup>
        </Stack>
        <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="filter/setup_filter" value={true}>Set Filter</EndpointButton>
        </Stack>
      </TitleCard>
      </Col>
      </Row>
      <Row>
        <Col md="3">
          <TitleCard title="Trigger">
              <Stack gap={stack_gap}>
                <Stack direction="horizontal" gap={stack_gap}>
                  <InputGroup>
                    <InputGroup.Text>Threshold</InputGroup.Text>
                    <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="trigger/settings/thres" />
                  </InputGroup>
                </Stack>
                <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="trigger/setup_trigger" value={true} >Set Trigger</EndpointButton>
              </Stack>
          </TitleCard>
        </Col>
        <Col>
          <TitleCard title="Base Subtraction">
            <Stack gap={stack_gap}>
              <Stack direction="horizontal" gap={stack_gap}>
                <Col xs="auto">
                <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="base_sub/settings/use_fixed"
                                checked={base_sub_data.settings.use_fixed || false} label="Use Fixed Value"
                                />
                </Col>
                <Col>
                <InputGroup>
                  <InputGroup.Text>Fixed Value</InputGroup.Text>
                  <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="base_sub/settings/fixed" />
                </InputGroup>
                </Col>
              </Stack>
              <Stack direction="horizontal" gap={stack_gap}>
              <EndpointDropdown endpoint={ngpdEndpoint} event_type="select" fullpath="base_sub/settings/div_cont" buttonText={base_sub_type}>
                {base_sub_data.div_cont_options ? base_sub_data.div_cont_options.map(
                  (div_cont, index) => (
                    <Dropdown.Item eventKey={index} key={div_cont} active={base_sub_type === div_cont}>{div_cont}</Dropdown.Item>
                  )) : <></>}
              </EndpointDropdown>
              <InputGroup>
              <InputGroup.Text>Error Limit</InputGroup.Text>
              <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="base_sub/settings/error_limit" />
              </InputGroup>
              </Stack>
              <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="base_sub/setup_base_sub" value={true}>Set Base Subtraction</EndpointButton>
            </Stack>
          </TitleCard>
        </Col>
      </Row>
      <Row>
        
        <Col>
          <TitleCard title="Pulse Measurement">
            <Stack gap={stack_gap}>
              
              <InputGroup>
                <InputGroup.Text>Tail Sum Delay (Bins)</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_sum_delay" />
                <InputGroup.Text>Tail Sum Number (Bins)</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_sum_num" />
              </InputGroup>
                <InputGroup>
                  <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="measure/settings/ignore_fall_time"
                                  checked={measure_data.ignore_fall_time || false} label="Ignore Fall Time" />
                  <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="measure/settings/ignore_tail_sum"
                                  checked={measure_data.ignore_tail_sum || false} label="Ignore Tail Sum" />
                </InputGroup>
                <InputGroup>
                  <InputGroup.Text>Fall Time Fraction</InputGroup.Text>
                    <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/fall_time_frac" />
                  </InputGroup>
                  <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="measure/setup_measure" value={true}>Set Pulse Measurement</EndpointButton>
            </Stack>
          </TitleCard>
        </Col>
        <Col>
          <TitleCard title="Neutron/Gamma Discrimination">
            <Stack gap={stack_gap}>
            <InputGroup>
                <InputGroup.Text>Minimum Height</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/min_height" />
                <InputGroup.Text>Maximum Height</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/max_height" />
            </InputGroup>
            <InputGroup>
                <InputGroup.Text>Minimum Fall Time</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/min_fall_time" />
                <InputGroup.Text>Maximum Fall Time</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/max_fall_time" />
            </InputGroup>
            <InputGroup>
                <InputGroup.Text>Neutron Tail Sum Minimum (thres c)</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_thres_c/all" />
                <InputGroup.Text>Neutron </InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_thres_m/all" />
            </InputGroup>
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="measure/setup_measure" value={true}>Set Pulse Discrimination</EndpointButton>
            </Stack>
          </TitleCard>
        </Col>
        </Row>
        <Row>
        <Col>
          <TitleCard title="ADC Range and Offset">
            <Stack gap={stack_gap}>
              <InputGroup>
                <InputGroup.Text>ADC Range</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="adc/range" />
                <InputGroup.Text>ADC Offset</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="adc/offset" />
              </InputGroup>
              <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="adc/setup_adc" value={true}>Setup ADC</EndpointButton>
            </Stack>
          </TitleCard>
        </Col>
        <Col>
        <TitleCard title="Scope Setup">
          <Stack gap={stack_gap}>
          <EndpointDropdown endpoint={ngpdEndpoint} event_type="select" fullpath="scope_options/scope_src"
                              buttonText={scope_src}>
            {ngpdEndpoint.data?.scope_src_options ? ngpdEndpoint.data.scope_src_options.map(
                  (scope_option, index) => (
                    <Dropdown.Item eventKey={index} key={scope_option} active={ngpdEndpoint.data?.scope_options.scope_src === index}>{scope_option}</Dropdown.Item>
                  )) : <></>}
          </EndpointDropdown>
          <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="scope_options/setup_scope_streams" value={true}>Setup Scope Streams</EndpointButton>
          </Stack>
        </TitleCard>
        </Col>
      </Row>
      </Container>
      <Container>
        <Row>
          <Col md="3">
        <TitleCard title="Scope Options">
          <Stack gap={stack_gap}>
            <InputGroup>
              <InputGroup.Text>Collection Time</InputGroup.Text>
              <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="scope_options/itfg/col_time" />
              <InputGroup.Text>(ms)</InputGroup.Text>
            </InputGroup>
            <InputGroup>
              <InputGroup.Text>Cycles</InputGroup.Text>
              <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="scope_options/itfg/cycles" />
            </InputGroup>
            
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="scope_options/start_scope" value={true}>
              Start Scope
            </EndpointButton>
            <hr/>
              <InputGroup>
              <InputGroup.Text>Data Points</InputGroup.Text>
              <Form.Control defaultValue={100} type="number" onChange={onChangeDataPoints}></Form.Control>

            </InputGroup>
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="data/refresh_data" value={data_points}>
              Get Data
            </EndpointButton>
            <InputGroup>
            {/* <InputGroup.Text>Save Data:</InputGroup.Text> */}
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="data/save_data" value={save_file}>
              Save Data
            </EndpointButton>
            <Form.Control type="text" placeholder="Filename" onChange={onChangeFileName}></Form.Control>
            </InputGroup>
            
          </Stack>
        </TitleCard>
        </Col>
        <Col md="9">
          <TitleCard title="Scope Data">
            <ScopeCanvas data={raw_data} isTimeBased={false} />
          </TitleCard>
        </Col>
        </Row>
      </Container>
    </OdinApp>
  )
}

export default App;
