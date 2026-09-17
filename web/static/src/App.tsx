import 'bootstrap/dist/css/bootstrap.min.css';

import { OdinApp, useAdapterEndpoint } from "@dssg/odin-react";
import { GraphPage } from './GraphPage';
import type { EndpointParams } from './types';
import { ConfigPage } from './ConfigurePage';


const App = () => {

  // Connect to the Odin Control Adapter you specified.
  // More endpoints for other adapters can be created.
  const endpoint = useAdapterEndpoint<EndpointParams>("ngpd", import.meta.env.VITE_ENDPOINT_URL);
  const data_endpoint = useAdapterEndpoint<{value: EndpointParams["acq"]["graph"]["hist"]}>("ngpd/acq/graph/hist", import.meta.env.VITE_ENDPOINT_URL);

  return (
    <OdinApp title="Neutron Gamma Pulse Discriminator"
    navLinks={["Configure", "Run and Display"]}>
      <ConfigPage endpoint={endpoint} />
      <GraphPage endpoint={endpoint} data_endpoint={data_endpoint}/>
    </OdinApp>
  )
}

export default App
