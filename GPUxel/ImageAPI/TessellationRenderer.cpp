


#include "pch.h"

#include "..\Content\Hvy3DScene.h"

#include "..\Common\DirectXHelper.h"

using namespace GPUxel;

using namespace DirectX;
using namespace Windows::Foundation;





        
void Hvy3DScene::gvConbuf_PrepareConbufData_Tessellation(unsigned int loopIndex_X, unsigned int loopIndex_Y)
{
    
    float xNOPO = e_PositionNOPO.x; //  centroid of the roving XRay monocle in NOPO coords;
    float yNOPO = e_PositionNOPO.y; //  centroid of the roving XRay monocle in NOPO coords;


    //  
    //  Convert from NOPO coords to Screen Coords. 
    //  Screen Coords have domain [0, 640] X [0, 480] etc...
    //    
    //  Remember maxWidthScreenCoords = m_deviceResources->GetOutputSize().Width;  //  screen width (ScreenCoords); 
    //    

    float xScreen = e_InputTextureSize.Width * (1.f + xNOPO) / 2.f; 
    float yScreen = e_InputTextureSize.Height * (1.f + yNOPO) / 2.f; 

    
    const double edgeLength = gv_GetUserSpecifiedEdgeLength();  //  originally was 150.00 when using 300 pixel inputImage;


    double px = xScreen;
    double py = yScreen - edgeLength / 2.0; 

    double rx = xScreen;
    double ry = yScreen + edgeLength / 2.0; 


    double dDistance = sqrt(   //  TODO:  calculation wants double-precision, conbuf doesn't support double-precision;
        (py - ry) * (py - ry) + (px - rx) * (px - rx)
    );


    double antiScale = dDistance / (e_EdgeCircle.GetCenter().x - e_EdgeCircle.GetRadius()); // reciprocal of dScale;


    double theta = atan2(ry - py, rx - px); //  Whereas "Mask" used theta = -atan2(ry - py, rx - px);



    //  Transform from hyperbolic space to input space: 

    AffineTransform   hyp2in;

    hyp2in.scale(antiScale);

    hyp2in.rotate(theta);

    hyp2in.translate(-px, -py);


    //  
    //  copy the six elements of the Affine Transformation: 
    //  


    e_ConBufTessData.hyp2in_ABC_.x = (float)hyp2in._a00; 
    e_ConBufTessData.hyp2in_ABC_.y = (float)hyp2in._a01; 
    e_ConBufTessData.hyp2in_ABC_.z = (float)hyp2in._a02; 
    e_ConBufTessData.hyp2in_ABC_.w = 0.0f;

    e_ConBufTessData.hyp2in_DEF_.x = (float)hyp2in._a10; 
    e_ConBufTessData.hyp2in_DEF_.y = (float)hyp2in._a11; 
    e_ConBufTessData.hyp2in_DEF_.z = (float)hyp2in._a12; 
    e_ConBufTessData.hyp2in_DEF_.w = 0.0f;


    //         
    //  Transform from output space to hyperbolic space:
    //         
    //  Strange things happen when the center is exactly on a pixel.  Perturb it slightly.
    //  TODO:  Old problem.  Check if this is still true.
    //         

    AffineTransform out2hyp;

    double iscale = 2.0 / (double)(renderedSize - 2 * borderSize - 1);
    out2hyp.scale(iscale);

    //  implicitly out2hyp.rotate is zero;

    double offset = -1.0 - iscale * borderSize + perturbationFactor;
    out2hyp.translate(-offset, -offset);


    e_ConBufTessData.out2hyp_ABC_.x = (float)out2hyp._a00; 
    e_ConBufTessData.out2hyp_ABC_.y = (float)out2hyp._a01; 
    e_ConBufTessData.out2hyp_ABC_.z = (float)out2hyp._a02; 
    e_ConBufTessData.out2hyp_ABC_.w = 0.0f;

    e_ConBufTessData.out2hyp_DEF_.x = (float)out2hyp._a10; 
    e_ConBufTessData.out2hyp_DEF_.y = (float)out2hyp._a11; 
    e_ConBufTessData.out2hyp_DEF_.z = (float)out2hyp._a12; 
    e_ConBufTessData.out2hyp_DEF_.w = 0.0f;


    //  Store details of the FDT Fundamental Domain Triangle 

    e_ConBufTessData.FDT.x = (float)e_FDT->_mSlope;  //  TODO:  D3D11 conbuf constant buffers don't support double-precision...
    e_ConBufTessData.FDT.y = (float)e_FDT->_geodesicEdge._circleRadius; 
    e_ConBufTessData.FDT.z = (float)e_FDT->_geodesicEdge._circleCenter.x; 
    e_ConBufTessData.FDT.w = (float)e_FDT->_geodesicEdge._circleCenter.y; 



    e_ConBufTessData.SchlafliSymbol.x = this->schlafli_p; 
    e_ConBufTessData.SchlafliSymbol.y = this->schlafli_q; 


    e_ConBufTessData.LoopIndex.x = loopIndex_X; 
    e_ConBufTessData.LoopIndex.y = loopIndex_Y;


	e_ConBufTessData.xu4a.x = e_ShowParity;  //  1 for true, zero for false; 
	e_ConBufTessData.xu4a.y = 0; // unused;
	e_ConBufTessData.xu4a.z = 0; // unused;
	e_ConBufTessData.xu4a.w = 0; // unused;

}












void Hvy3DScene::gv_DispatchComputeShaderTessellation()
{
	auto context = m_deviceResources->GetD3DDeviceContext();


    if (eInvalidated == true)
    {
        ID3D11ShaderResourceView* nullSRV[] = { nullptr }; // singleton array;
        context->PSSetShaderResources(0, 1, nullSRV);

        //      
        //  The following pair of "for" loops have upper bound = 4. 
        //  Four derives from the fact that the outputImage is 2048 pixels 
        //  on an edge and a Dispatch of 512 thread groups seems to work well, 
        //  so
        //          2048 / 512 = 4  <------ number of loop iterations; 
        //      

        for (unsigned int idx_X = 0; idx_X < 4; idx_X++)
        {
            for (unsigned int idx_Y = 0; idx_Y < 4; idx_Y++)
            {
                gvConbuf_PrepareConbufData_Tessellation(idx_X, idx_Y);
                context->UpdateSubresource1(e_ConBufTessBuffer.Get(), 0, NULL, &e_ConBufTessData, 0, 0, 0);

                context->CSSetShader(e_ComputeShaderTess.Get(), nullptr, 0);

                //  Bind input_READONLY_SRV, output_TESS_as_UAV and conbuf to the compute shader:  
                context->CSSetShaderResources(0, 1, input_READONLY_SRV.GetAddressOf());
                context->CSSetUnorderedAccessViews(0, 1, output_TESS_as_UAV.GetAddressOf(), nullptr);
                context->CSSetConstantBuffers(0, 1, e_ConBufTessBuffer.GetAddressOf());

                context->Dispatch(512, 512, 1); //  does 512 perform better than 2048 ?
            }
        }

        //  Un-bind output_as_UAV from the Compute Shader so that output_as_SRV can be bound to the Pixel Shader: 

        ID3D11UnorderedAccessView* nullUAV[] = { nullptr }; // singleton array;

        context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

        eInvalidated = false;
    }

    //      
    //     VS Vertex Shader: 
    //      

	context->UpdateSubresource1( e_ConBufTessWVPBuffer.Get(), 0, NULL, &e_ConBufTessWVPData, 0, 0, 0 );

	UINT stride = sizeof(VHG_CubeFaceVertexPT);
	UINT offset = 0;
	context->IASetVertexBuffers( 0, 1, m_MonoQuadVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
	context->IASetIndexBuffer( m_MonoQuadIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
	context->VSSetConstantBuffers1( 0, 1, e_ConBufTessWVPBuffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

	context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, output_TESS_as_SRV.GetAddressOf()); // Pixel Shader must be able to read the output image;
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());



#ifdef GHV_OPTION_DUAL_VIEWPORTS

    context->RSSetViewports(1, &e_ViewportMajor);

#endif


	context->DrawIndexed( m_MonoQuadIndexCount, 0, 0 );
}








Windows::Foundation::Size Hvy3DScene::gvCreateOutputResourceViewsForTess()
{
    output_D3D11_Resource.Reset();


    wchar_t file_path_to_image[] = L"Assets\\0_Output_Image_Surrogate_1200.png"; // Super Speed version uses 2048x2048 here;


    HRESULT hrOutputSurrogate = CreateWICTextureFromFileEx(
        m_deviceResources->GetD3DDevice(), 
        file_path_to_image, 
        0,  //  maxsize;
        D3D11_USAGE_DEFAULT,   //  the D3D11_USAGE; 
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,  //  the bindFlags (unsigned int); 
        0,   //  cpuAccessFlags; 
        0,   //  miscFlags;  
        WIC_LOADER_DEFAULT,   //  loadFlags   (can be _DEFAULT, _FORCE_SRGB or WIC_LOADER_IGNORE_SRGB); 
        output_D3D11_Resource.GetAddressOf(),  // the Texture being created; 
        nullptr   //  usually srv_something.GetAddressOf(), 
    ); 

    DX::ThrowIfFailed(hrOutputSurrogate);


    //  
    //  Reinterpret the D3D11 Resource as a Texture2D: 
    //  

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hrTex = output_D3D11_Resource.As(&tex);
    DX::ThrowIfFailed(hrTex);


    //  Create a readable resource view on the output image: 

    CD3D11_SHADER_RESOURCE_VIEW_DESC output_as_SRV_descr(tex.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
        output_D3D11_Resource.Get(), &output_as_SRV_descr, &output_TESS_as_SRV)
    );

    //  Create a writable resource view on the output image: 

    CD3D11_UNORDERED_ACCESS_VIEW_DESC output_as_UAV_descr(tex.Get(), D3D11_UAV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(
        output_D3D11_Resource.Get(), &output_as_UAV_descr, &output_TESS_as_UAV)
    );

    //  
    //  Get the size of the Texture2D: 
    //  

    D3D11_TEXTURE2D_DESC descTexture2D;
    tex->GetDesc(&descTexture2D);

    Windows::Foundation::Size outputTextureSize;
    outputTextureSize.Width = (float)descTexture2D.Width;
    outputTextureSize.Height = (float)descTexture2D.Height;

    return outputTextureSize; 
}






