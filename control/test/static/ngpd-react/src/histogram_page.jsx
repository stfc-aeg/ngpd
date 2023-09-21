import React, { useMemo } from "react";

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

import { TitleCard, DropdownSelector, ToggleSwitch, WithEndpoint, OdinGraph } from "odin-react";
import { Buffer } from "buffer";

const EndpointButton = WithEndpoint(Button);
const EndpointInput = WithEndpoint(Form.Control);
const EndpointDropdown = WithEndpoint(DropdownSelector);
const EndpointToggle = WithEndpoint(ToggleSwitch);

function HistogramPage(props) {
    const {ngpdEndpoint, stack_gap} = props;

    const [hist_data, changeHistData] = useState([{}]);
    const [hist_type, changeHistType] = useState({});
    const [hist_dropdown_text, changeHistDropdownText] = useState("None");
    const [graph_width, changeGraphWidth] = useState();
    const [multiple_graph, setMultipleGraph] = useState(false);

    const hist_select_options = useMemo(() => {return {"Height": "height",
    "Tail Sum": "tail_sum",
    "Fall Time": "fall_time",
    "Tail Ratio": "tail_ratio",
    "Height X Tail Sum": "height_tail_sum",
    "Height X Fall Time": "height_fall_time",
    "Height X Tail Ratio": "height_tail_ratio"}}, []);

    const decode64Data = (encoded_data) => {
      if(encoded_data)
      {
        console.log(encoded_data);
        var uint8_buffer = Uint8Array.from(Buffer.from(encoded_data, "base64"));
        return new Uint32Array(uint8_buffer.buffer);
      }
      else
      {
        return null;
      }
    }

    const renderGraph = () => {
      if(multiple_graph)
      {
        return (
          <Row>
            <OdinGraph prop_data={hist_data[0]} type={hist_type} title={hist_dropdown_text + ": 1"} num_x={graph_width}/>
            <OdinGraph prop_data={hist_data[1]} type={hist_type} title={hist_dropdown_text + ": 2"} num_x={graph_width}/>
            <OdinGraph prop_data={hist_data[2]} type={hist_type} title={hist_dropdown_text + ": 3"} num_x={graph_width}/>
          </Row>
        )
      }
      else
      {
        return <OdinGraph prop_data={hist_data} type={hist_type} title={hist_dropdown_text} num_x={graph_width}/>
      }
    }
    useEffect(() => {
      for(var key in hist_select_options)
      {
        if((ngpdEndpoint.data.histogram?.hist_select || null) === hist_select_options[key])
        {
          changeHistDropdownText(key);
          break;
        }
      }
    }, [ngpdEndpoint.data.histogram?.hist_select, hist_select_options])

    useEffect(() => {
        console.log("Changing Hist Data");

        var data = decode64Data(ngpdEndpoint.data.histogram?.data);
        console.log(data);
    
        if(data == null)
        {
          console.log("no hist data available");
          return;
        }
        var dims = ngpdEndpoint.data.histogram?.data_shape;
        console.log(dims);

        
    
        // // var type = "";
        // var hist_data = [];
        // var buttons = [];

        var reshaped_data = [];
        if(dims[1] === 1)
        {
          //1d data
          setMultipleGraph(false);
          changeHistType("scatter");
          if(dims[2] > 1)
          {
            //need to split data into the multiple datasets
            // var reshaped_data = [];
            for(var i = 0; i<data.length; i+=dims[0])
            {
              reshaped_data.push(Array.from(data.slice(i, i + dims[0])));
            }
            console.log(reshaped_data);
            changeHistData(reshaped_data);
          }
          else
          {
            changeHistData(data);
          }


        }
        else
        {
          //2d data
          changeHistType("heatmap");
          changeGraphWidth(dims[0]);
          

          if(dims[2] > 1)
          {
            // var reshaped_data = [];
            setMultipleGraph(true);
            const dataset_length = dims[0] * dims[1];
            for(var j = 0; j<data.length; j+=dataset_length)
            {
              reshaped_data.push(Array.from(data.slice(j, j + dataset_length)));
            }
            //we need multiple plots I guess?
            changeHistData(reshaped_data);
            
          }
          else
          {
            setMultipleGraph(false);
            changeHistData(data);
          }
        }
      }, [ngpdEndpoint.data?.histogram?.data, ngpdEndpoint.data?.histogram?.hist_select, ngpdEndpoint.data.histogram?.data_shape])

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
            {renderGraph()}
          </TitleCard>
        </Col>
        </Row>
      </Container>
    )
}

export default HistogramPage;