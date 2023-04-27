
import React, { useMemo, useState } from 'react';

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
  const dataEndpoint = useAdapterEndpoint("ngpd/data", process.env.REACT_APP_ENDPOINT_URL, 1000);

  const setup_data = ngpdEndpoint.data?.setup ? ngpdEndpoint.data.setup : {};
  const filter_data = ngpdEndpoint.data?.filter ? ngpdEndpoint.data.filter : {};
  const trigger_data = ngpdEndpoint.data?.trigger ? ngpdEndpoint.data.trigger.settings : {};
  const base_sub_data = ngpdEndpoint.data?.base_sub ? ngpdEndpoint.data.base_sub : {settings: {}};
  const measure_data = ngpdEndpoint.data?.measure ? ngpdEndpoint.data.measure.settings : {};
  const adc_data = ngpdEndpoint.data?.adc ? ngpdEndpoint.data.adc : {};

  const base_sub_type = ngpdEndpoint.data?.base_sub ? base_sub_data.div_cont_options[base_sub_data.settings.div_cont] : "Unknown";

  const stack_gap = 2

  const [data_points, changeDataPoints] = useState(100);

  const onChangeDataPoints = (event) => {
    console.log(event)
    changeDataPoints(+event.target.value);
  }

  const raw_data =  [{label: "Raw Scope Data",
                    x: dataEndpoint.data?.raw_data ? Array.from(dataEndpoint.data.raw_data, (_, i) => i): [0, 1],
                    y: dataEndpoint.data ? dataEndpoint.data.raw_data : [0, 1]
                   }];

  // const test_data = [{label: "Test",
  //                    x: ngpdEndpoint.data?.data ? Array.from(ngpdEndpoint.data.data.raw_data, (_, i) => i) : [0],
  //                    y: ngpdEndpoint.data?.data ? ngpdEndpoint.data.data.raw_data : [0]}]

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
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="setup/path" value={setup_data.path || 0} readOnly disabled></EndpointInput>
              </InputGroup>
              <InputGroup>
                <InputGroup.Text>Channel</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="setup/channel" value={setup_data.channel || 0} readOnly disabled></EndpointInput>
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
            <EndpointDropdown endpoint={ngpdEndpoint} event_type="select" fullpath="filter/type" buttonText={filter_data.type}>
              {filter_data.type_options ? filter_data.type_options.map(
                (selected_type) => (
                  <Dropdown.Item eventKey={selected_type} key={selected_type} active={filter_data.type === selected_type}>{selected_type}</Dropdown.Item>
                )) : <></>
              }
            </EndpointDropdown>
          </Col>
          {/* </Stack> */}
        <Stack direction="horizontal" gap={stack_gap}>
          <InputGroup>
            <InputGroup.Text>T Samples</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} fullpath="filter/tsamples" value={filter_data.tsamples || 0} disabled={!(filter_data.type==="exp")} />
          </InputGroup>
          <InputGroup>
            <InputGroup.Text>Num Average</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="filter/num_ave" value={filter_data.num_ave || 0} disabled={!(filter_data.type==="ave")}/>
          </InputGroup>
          <InputGroup>
            <InputGroup.Text>Sigma</InputGroup.Text>
            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="filter/sigma" value={filter_data.sigma || 0} disabled={!(filter_data.type==="gaussian")}/>
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
                    <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="trigger/settings/thres" defaultValue={trigger_data.thres || 0} />
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
                  <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="base_sub/settings/fixed" value={base_sub_data.settings.fixed || 0}/>
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
              <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="base_sub/settings/error_limit" value={base_sub_data.settings.error_limit || 0}/>
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
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_sum_delay" value={measure_data.tail_sum_delay || 0} />
                <InputGroup.Text>Tail Sum Number (Bins)</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_sum_num" value={measure_data.tail_sum_num || 0} />
              </InputGroup>
                <InputGroup>
                  <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="measure/settings/ignore_fall_time"
                                  checked={measure_data.ignore_fall_time || false} label="Ignore Fall Time" />
                  <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="measure/settings/ignore_tail_sum"
                                  checked={measure_data.ignore_tail_sum || false} label="Ignore Tail Sum" />
                </InputGroup>
                <InputGroup>
                  <InputGroup.Text>Fall Time Fraction</InputGroup.Text>
                    <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/fall_time_frac" value={measure_data.fall_time_frac || 0} />
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
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/min_height" value={measure_data.min_height || 0} />
                <InputGroup.Text>Maximum Height</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/max_height" value={measure_data.max_height || 0} />
            </InputGroup>
            <InputGroup>
                <InputGroup.Text>Minimum Fall Time</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/min_fall_time" value={measure_data.min_fall_time || 0} />
                <InputGroup.Text>Maximum Fall Time</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/max_fall_time" value={measure_data.max_fall_time || 0} />
            </InputGroup>
            <InputGroup>
                <InputGroup.Text>Neutron Tail Sum Minimum (thres c)</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_thres_c/all" value={measure_data.tail_thres_c?.all || 0} />
                <InputGroup.Text>Neutron </InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="measure/settings/tail_thres_m/all" value={measure_data.tail_thres_m?.all || 0} />
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
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="adc/range" value={adc_data.range} />
                <InputGroup.Text>ADC Offset</InputGroup.Text>
                <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="adc/offset" value={adc_data.offset} />
              </InputGroup>
              <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="adc/setup_adc" value={true}>Setup ADC</EndpointButton>
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
            
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="scope_options/start_scope">
              Start Scope
            </EndpointButton>
              <InputGroup>
              <InputGroup.Text>Data Points</InputGroup.Text>
              <Form.Control defaultValue={100} type="number" onChange={onChangeDataPoints}></Form.Control>

            </InputGroup>
            <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="data/refresh_data" value={data_points}>
              Refresh
            </EndpointButton>
          </Stack>
        </TitleCard>
        </Col>
        <Col md="9">
          <TitleCard title="Scope Data">
            <ScopeCanvas data={raw_data} isTimeBased={false} />
            {/* <ScopeCanvas data={test_data} isTimeBased={false} /> */}
          </TitleCard>
        </Col>
        </Row>
      </Container>
    </OdinApp>
  )
}

export default App;
