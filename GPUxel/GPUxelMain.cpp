#include "pch.h"
#include "GPUxelMain.h"
#include "Common\DirectXHelper.h"

using namespace GPUxel;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;














GPUxelMain::GPUxelMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);


	m_sceneRenderer = std::unique_ptr<Hvy3DScene>(new Hvy3DScene(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<HUD>(new HUD(m_deviceResources));



	//  Rather than the default variable timestep, 
    //  specify fixed timestep in FPS: 


	m_timer.SetFixedTimeStep(true);

	m_timer.SetTargetElapsedSeconds(1.0 / 30); // Super Speed version uses 30 frames per second; 
}











GPUxelMain::~GPUxelMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}















// Updates application state when the window size changes (e.g. device orientation change)
void GPUxelMain::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}












void GPUxelMain::Update() 
{

	m_timer.Tick([&]()
	{

		m_sceneRenderer->Update(m_timer);

		//    undo   m_fpsTextRenderer->Update(m_timer);
	});
}















// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.



bool GPUxelMain::Render() 
{
	// Don't try to render anything before the first Update.


	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}


	auto context = m_deviceResources->GetD3DDeviceContext();


	// Reset the viewport to target the whole screen.

	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);



	// Reset render targets to the screen.

	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };

	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());




	// Clear the back buffer and depth stencil view.

	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);

	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);





	m_sceneRenderer->Render();
	
    
    //  undo :  m_fpsTextRenderer->Render();

	return true;
}

















// Notifies renderers that device resources need to be released.
void GPUxelMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void GPUxelMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
