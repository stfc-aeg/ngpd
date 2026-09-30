import 'bootstrap/dist/css/bootstrap.min.css';

import { OdinApp, useAdapterEndpoint } from "@dssg/odin-react";
import { GraphPage } from './GraphPage';
import type { EndpointParams } from './types';
import { ConfigPage } from './ConfigurePage';
import { MonitorPage } from './MonitorPage';


const App = () => {

  // Connect to the Odin Control Adapter you specified.
  // More endpoints for other adapters can be created.
  const endpoint = useAdapterEndpoint<EndpointParams>("ngpd", import.meta.env.VITE_ENDPOINT_URL, 500);

  // endpoint for the Hist data, as that wont send the full data unless directly requested. this avoids lag in the main endpoint
  const data_endpoint = useAdapterEndpoint<{value: EndpointParams["acq"]["graph"]["hist"]}>("ngpd/acq/graph/hist", import.meta.env.VITE_ENDPOINT_URL);

  return (
    <OdinApp title="Neutron Gamma Pulse Discriminator"
    navLinks={["Configure", "Monitor", "Run and Display"]}>
      <ConfigPage endpoint={endpoint} />
      <MonitorPage endpoint={endpoint} />
      <GraphPage endpoint={endpoint} data_endpoint={data_endpoint}/>
    </OdinApp>
  )
}

export default App
