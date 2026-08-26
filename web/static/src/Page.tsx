import { TitleCard, ParamController } from "@dssg/odin-react";
import type { AdapterEndpoint } from "@dssg/odin-react";
import { Container, Row, Col } from 'react-bootstrap';
import { type EndpointParams } from "./App";

interface PageProps {
    endpoint: AdapterEndpoint<EndpointParams>
}

const Page = ({
    endpoint
}: PageProps) => {

    return (
        <Container>
            <Row>
                <Col>
                    <TitleCard title="Demo">
                        A Basic page using Bootstrap's Row/Col grid layout.
                        Use this as a starting point for your GUI.
                        <br />
                        Below is an auto-generated set of controls for your
                        adapter to test and confirm the connection is working.
                        This should not be used in the final GUI and is for debug
                        purposes only
                    </TitleCard>
                </Col>
            </Row>
            <Row>
                <Col>
                    <ParamController endpoint={endpoint} title="ngpd"/>
                </Col>
            </Row>
        </Container>
    )
}

export default Page;