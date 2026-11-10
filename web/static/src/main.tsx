import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App.tsx'
import { OdinErrorContext } from '@dssg/odin-react'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <OdinErrorContext>
      <App />
    </OdinErrorContext>
  </StrictMode>,
)
