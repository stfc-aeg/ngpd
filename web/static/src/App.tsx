import 'bootstrap/dist/css/bootstrap.min.css';

import { OdinApp, useAdapterEndpoint } from "@dssg/odin-react";
import Page from "./Page";

/**An Interface to define the shape of the Parameter Tree from the Odin Control Adapter.
* Define Parameter Names and Types here to allow your IDE to know what they are
* if and when accessing the data within your App
* */
export interface EndpointParams {
  /* Add any Parameters you'll be using to this interface, such as this example*/
  example: string;
}

const App = () => {

  // Connect to the Odin Control Adapter you specified.
  // More endpoints for other adapters can be created.
  const endpoint = useAdapterEndpoint<EndpointParams>("ngpd", import.meta.env.VITE_ENDPOINT_URL);

  return (
    <OdinApp title="NGPD">
      <Page endpoint={endpoint}/>
    </OdinApp>
  )
}

export default App
