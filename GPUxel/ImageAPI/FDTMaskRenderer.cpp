//          
//      
//  FDTMaskRenderer.cpp
//          
//


#include "pch.h"

#include "..\Content\Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace GPUxel;

using namespace DirectX;
using namespace Windows::Foundation;





        
void Hvy3DScene::gvConbuf_PrepareConbufData_Mask()
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




    //               
    //  Definition of "edgeLength" for FDT Mask: 
    //
    //  The constant "edgeLength" specifies the length in pixels 
    //  of one side of the fundamentail domain triangle (FDT). 
    //  
    //  The FDT is a hyperbolic right triangle whose sides are: 
    //      1. The hypotenuse, a Euclidean-straight line segment;
    //      2. The hyperbolic geodesic arc segment; 
    //      3. Another Euclidean-straight line segment. 
    //
    //  "edgeLength" specifies #3 in the list above. 
    //      

    const double edgeLength = gv_GetUserSpecifiedEdgeLength();  //  originally was 150.00 when using 300 pixel inputImage;

    double px = xScreen;
    double py = yScreen - edgeLength / 2.0; 

    double rx = xScreen;
    double ry = yScreen + edgeLength / 2.0; 


    double dDistance = sqrt(   //  TODO:  calculation wants double-precision, conbuf doesn't support double-precision;
        (py - ry) * (py - ry) + (px - rx) * (px - rx)
    );

    double dScale = (e_EdgeCircle.GetCenter().x - e_EdgeCircle.GetRadius()) / dDistance;

    double theta = -atan2(ry - py, rx - px);

    AffineTransform   tAffine;

    tAffine.translate(px, py);
    tAffine.rotate(theta);
    tAffine.scale(dScale);
    
    //  
    //  copy the six elements of the Affine Transformation: 
    //  
    //  TODO: D3D11 conbuf constant buffers don't support double-precision...
    //  

    e_ConBufMaskData.AffineABC_.x = (float)tAffine._a00; 
    e_ConBufMaskData.AffineABC_.y = (float)tAffine._a01; 
    e_ConBufMaskData.AffineABC_.z = (float)tAffine._a02; 
    e_ConBufMaskData.AffineABC_.w = 0.0f;

    e_ConBufMaskData.AffineDEF_.x = (float)tAffine._a10; 
    e_ConBufMaskData.AffineDEF_.y = (float)tAffine._a11; 
    e_ConBufMaskData.AffineDEF_.z = (float)tAffine._a12; 
    e_ConBufMaskData.AffineDEF_.w = 0.0f;
   
    //  Store details of the FDT Fundamental Domain Triangle 
    //  in e_ConBufMaskData.FDT: 
   
    e_ConBufMaskData.FDT.x = (float)e_FDT->_mSlope;  //  TODO:  D3D11 conbuf constant buffers don't support double-precision...
    e_ConBufMaskData.FDT.y = (float)e_FDT->_geodesicEdge._circleRadius; 
    e_ConBufMaskData.FDT.z = (float)e_FDT->_geodesicEdge._circleCenter.x; 
    e_ConBufMaskData.FDT.w = (float)e_FDT->_geodesicEdge._circleCenter.y; 
}








void Hvy3DScene::gv_DispatchComputeShaderFDTMask()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    gvConbuf_PrepareConbufData_Mask();
    context->UpdateSubresource1( e_ConBufMaskBuffer.Get(), 0, NULL, &e_ConBufMaskData, 0, 0, 0 );

    context->CSSetShader(e_ComputeShaderFDTMask.Get(), nullptr, 0);

    //  Bind input_READONLY_SRV, output_MASK_as_UAV and conbuf to the compute shader:  

    context->CSSetShaderResources(0, 1, input_READONLY_SRV.GetAddressOf()); 
    context->CSSetUnorderedAccessViews(0, 1, output_MASK_as_UAV.GetAddressOf(), nullptr);
    context->CSSetConstantBuffers(0, 1, e_ConBufMaskBuffer.GetAddressOf()); 

    context->Dispatch(100, 100, 1);

    //  Un-bind output_as_UAV from the CS Compute Shader so that output_as_SRV can be bound to the Pixel Shader: 

    ID3D11UnorderedAccessView* nullUAV[] = { nullptr }; // singleton array;
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);


    //      
    //     VS Vertex Shader: 
    //      

    context->UpdateSubresource1( e_ConBufMaskWVPBuffer.Get(), 0, NULL, &e_ConBufMaskWVPData, 0, 0, 0 );

    UINT stride = sizeof(VHG_CubeFaceVertexPT);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, m_TextureCubeVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
    context->IASetIndexBuffer( m_TextureCubeIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, e_ConBufMaskWVPBuffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

    context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, output_MASK_as_SRV.GetAddressOf()); // Pixel Shader must be able to read the output image;
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());

#ifdef GHV_OPTION_DUAL_VIEWPORTS
    context->RSSetViewports(1, &e_ViewportMinor);
#endif

    context->DrawIndexed( m_TextureCubeIndexCount, 0, 0 );

    //  Un-bind: 
    ID3D11ShaderResourceView* nullSRV[] = { nullptr }; // singleton array;
    context->PSSetShaderResources(0, 1, nullSRV);
}








        
void Hvy3DScene::gv_FDTMaskRenderPlain()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    //      
    //     VS Vertex Shader: 
    //      

    context->UpdateSubresource1( e_ConBufMaskWVPBuffer.Get(), 0, NULL, &e_ConBufMaskWVPData, 0, 0, 0 );

    UINT stride = sizeof(VHG_CubeFaceVertexPT);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, m_TextureCubeVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
    context->IASetIndexBuffer( m_TextureCubeIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, e_ConBufMaskWVPBuffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

    context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, input_READONLY_SRV.GetAddressOf()); // !!! hazard undo experimental !!!
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());

#ifdef GHV_OPTION_DUAL_VIEWPORTS
    context->RSSetViewports(1, &e_ViewportMinor);
#endif

    context->DrawIndexed( m_TextureCubeIndexCount, 0, 0 );
}





Windows::Foundation::Size Hvy3DScene::gvCreateOutputResourceViewsForFDTMask()
{
    Microsoft::WRL::ComPtr<ID3D11Resource>  temp_resource;


    wchar_t file_path_to_image[] = L"Assets\\0_Output_Image_Surrogate_300.png"; // Super Speed version uses 1024x1024 here; 

    HRESULT hrOutputSurrogate = CreateWICTextureFromFileEx(
        m_deviceResources->GetD3DDevice(), 
        file_path_to_image, 
        0,  //  maxsize;
        D3D11_USAGE_DEFAULT,   //  the D3D11_USAGE; 
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,  //  the bindFlags (unsigned int); 
        0,   //  cpuAccessFlags; 
        0,   //  miscFlags;  
        WIC_LOADER_DEFAULT,   //  loadFlags   (can be _DEFAULT, _FORCE_SRGB or WIC_LOADER_IGNORE_SRGB); 
        temp_resource.GetAddressOf(),  // the Texture being created; 
        nullptr   //  usually srv_something.GetAddressOf(), 
    ); 

    DX::ThrowIfFailed(hrOutputSurrogate);

    //  
    //  Reinterpret the D3D11 Resource as a Texture2D: 
    //  

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hrTex = temp_resource.As(&tex);
    DX::ThrowIfFailed(hrTex);

    //  Create a readable resource view on the output image: 

    CD3D11_SHADER_RESOURCE_VIEW_DESC output_as_SRV_descr(tex.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
        temp_resource.Get(), &output_as_SRV_descr, &output_MASK_as_SRV)
    );

    //  Create a writable resource view on the output image: 

    CD3D11_UNORDERED_ACCESS_VIEW_DESC output_as_UAV_descr(tex.Get(), D3D11_UAV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(
        temp_resource.Get(), &output_as_UAV_descr, &output_MASK_as_UAV)
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







