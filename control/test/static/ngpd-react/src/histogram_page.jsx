import React from "react";

import { useState, useEffect } from "react";

import Container from 'react-bootstrap/Container';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import Stack from 'react-bootstrap/Stack';
import Accordion from 'react-bootstrap/Accordion';
import Button from 'react-bootstrap/Button';
import Form from 'react-bootstrap/Form';
import Dropdown from 'react-bootstrap/Dropdown';
import InputGroup from 'react-bootstrap/InputGroup';


import Plot from 'react-plotly.js';

import { TitleCard, DropdownSelector, ToggleSwitch, WithEndpoint } from "odin-react";

const EndpointButton = WithEndpoint(Button);
const EndpointInput = WithEndpoint(Form.Control);
const EndpointDropdown = WithEndpoint(DropdownSelector);
const EndpointToggle = WithEndpoint(ToggleSwitch);

function HistogramPage(props) {
    const {ngpdEndpoint, stack_gap} = props;

    const [hist_data, changeHistData] = useState([{}]);
    const [hist_layout, changeHistLayout] = useState({});
    const [hist_dropdown_text, changeHistDropdownText] = useState("None");
    const [updateMenu, changeUpdateMenu] = useState({});

    const hist_select_options = {"Height": "height",
    "Tail Sum": "tail_sum",
    "Fall Time": "fall_time",
    "Tail Ratio": "tail_ratio",
    "Height X Tail Sum": "height_tail_sum",
    "Height X Fall Time": "height_fall_time",
    "Height X Tail Ratio": "height_tail_ratio"};

    useEffect(() => {
        console.log("Changing Hist Data");
        var data = ngpdEndpoint.data.histogram?.data || null;
    
        if(data == null)
        {
          console.log("no hist data available");
          return;
        }
    
        var x_dim = data.length;
        var y_dim = data[0].length;
        var t_dim = data[0][0].length;
        console.log("(" + x_dim + ", " + y_dim + ", " + t_dim + ")");
    
        // var type = "";
        var hist_data = [];
        var buttons = [];
        if(y_dim === 1)
        {
          // type = "scatter";
          for(var i = 0; i<t_dim; i++)
          {
            var plot_data ={
              x: Array.from(data, (_, k) => k),
              y: Array.from(data, (v, k) => v[0][i]),
              type: "scatter"
            }
            hist_data.push(plot_data);
          }
          changeHistLayout({yaxis: {autorange: true}, width:1, height:1});
          changeUpdateMenu(null);
        }
        else
        {
            for(var i = 0; i<t_dim; i++)
            {
            var plot_data ={
              z: Array.from(data, (x, k) => Array.from(x, (y, k) => y[i])),
              type: "heatmap",
              xaxis:"x",
              yaxis: "y"
            }
            hist_data.push(plot_data);
            var visible_array = new Array(t_dim).fill(false);
            visible_array[i] = true;
            var button = {args: [{'visible': visible_array}], method: 'update'}
            buttons.push(button);
          }
          changeHistLayout({zaxis: { type: 'log', autorange: true}, width:x_dim, height:y_dim,
                            updatemenus: 
                            [{
                                buttons: buttons,
                                direction: 'left',
                                showactive: true,
                                type: "buttons"
                            }]
        });
          
          
        }
    
        for(var key in hist_select_options)
        {
          // console.log(key + ": " + hist_select_options[key]);
          // console.log(ngpdEndpoint.data.histogram?.hist_select);
          if((ngpdEndpoint.data.histogram?.hist_select || null) === hist_select_options[key])
          {
            changeHistDropdownText(key);
            break;
          }
        }
        changeHistData(hist_data);
      }, [ngpdEndpoint.data?.histogram?.data, ngpdEndpoint.data?.histogram?.hist_select])

    return (
<Container>
        <Row>
        <Col>
          <TitleCard title="Histogram Options">
            <Row>
                <Col md="4">
                <Accordion defaultActiveKey="0">
                  <Accordion.Item eventKey="0">
                  <Accordion.Header>Enable Histograms</Accordion.Header>
                  <Accordion.Body>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/height"
                                    checked={ngpdEndpoint.data.histogram?.enable.height || false} label="Height"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/tail_sum"
                                    checked={ngpdEndpoint.data.histogram?.enable.tail_sum || false} label="Tail Sum"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/fall_time"
                                    checked={ngpdEndpoint.data.histogram?.enable.fall_time || false} label="Fall Time"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/tail_ratio"
                                    checked={ngpdEndpoint.data.histogram?.enable.tail_ratio || false} label="Tail Ratio"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/height_tail_sum"
                                    checked={ngpdEndpoint.data.histogram?.enable.height_tail_sum || false} label="Height by Tail Sum"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/height_fall_time"
                                    checked={ngpdEndpoint.data.histogram?.enable.height_fall_time || false} label="Height by Fall Time"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/height_tail_ratio"
                                    checked={ngpdEndpoint.data.histogram?.enable.height_tail_ratio || false} label="Height by Tail Ratio"/>
                    <hr/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/separate_ngp"
                                    checked={ngpdEndpoint.data.histogram?.enable.separate_ngp || false} label="Separate NGP"/>
                    <EndpointToggle endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/enable/discard_pileup"
                                    checked={ngpdEndpoint.data.histogram?.enable.discard_pileup || false} label="Discard Pileup Events"/>
                  </Accordion.Body>
                  </Accordion.Item>
                </Accordion>
                </Col>
                <Col>
                    <Stack gap={stack_gap}>
                    <Stack direction="horizontal" gap={stack_gap}>
                        <InputGroup>
                            <InputGroup.Text>Height Bins</InputGroup.Text>
                            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="histogram/options/height" />
                        </InputGroup>
                    
                        <InputGroup>
                        <InputGroup.Text>Tail Sum Bins</InputGroup.Text>
                            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="histogram/options/tail_sum" />
                        </InputGroup>
                    
                        <InputGroup>
                        <InputGroup.Text>Fall Time Bins</InputGroup.Text>
                            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="histogram/options/fall_time" />
                        </InputGroup>
                    </Stack>
                    <Stack direction="horizontal" gap={stack_gap}>
                        <InputGroup>
                        <InputGroup.Text>Tail Ratio Bins</InputGroup.Text>
                            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="histogram/options/tail_ratio" />
                        </InputGroup>
                        <InputGroup>
                        <InputGroup.Text>Max Ratio</InputGroup.Text>
                            <EndpointInput endpoint={ngpdEndpoint} type="number" fullpath="histogram/options/max_ratio" />
                        </InputGroup>
                    </Stack>
                
                <Stack direction="horizontal" gap={stack_gap}>
                <EndpointButton endpoint={ngpdEndpoint} event_type="click" fullpath="histogram/setup" value={true}>
                  Setup Histogramming
                </EndpointButton>
                <InputGroup>
                <InputGroup.Text>Histogram Display:</InputGroup.Text>
                  <EndpointDropdown endpoint={ngpdEndpoint} event_type="select" fullpath="histogram/hist_select"
                                    buttonText={hist_dropdown_text}>
                    
                    {ngpdEndpoint.data.histogram?.enable ? Object.keys(hist_select_options).map(
                      (hist_select, index) => (
                        
                        <Dropdown.Item eventKey={hist_select_options[hist_select]}
                                       key={hist_select_options[hist_select]}
                                       active={ngpdEndpoint.data.histogram?.hist_select === hist_select_options[hist_select]}
                                       disabled={!ngpdEndpoint.data.histogram?.enable[hist_select_options[hist_select]]}
                        >{hist_select}</Dropdown.Item>
                        
                  )) : <></>}
                  </EndpointDropdown>
                </InputGroup>
                </Stack>
                </Stack>
                
                  

                
                
                </Col>
            </Row>
          </TitleCard>
          <TitleCard title="Histogram Data">
            <Plot
              data={hist_data}
              layout={hist_layout}
              config={{"responsive": true}}/>
          </TitleCard>
        </Col>
        </Row>
      </Container>
    )
}

export default HistogramPage;