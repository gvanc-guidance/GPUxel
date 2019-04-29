#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"






namespace GPUxel
{
    using VHG_NOPO = DirectX::XMFLOAT2; 


    using HvyPlex = std::complex<double>;


    const static double PI = 4.0*atan(1.0);







    typedef struct
    {
        double x;
        double y;
    } BHPPoint;






    class BHPCircle
    {
    public:

        BHPCircle();


        BHPCircle(BHPPoint circleCenter, double circleRadius);


        double GetRadius() { return _circleRadius; }


        BHPPoint GetCenter() { return _circleCenter; }


        bool insideOrOn(BHPPoint somePoint);


        BHPPoint invert(BHPPoint somePoint);


    public:

        BHPPoint _circleCenter;  //  formerly "_p";

        double _circleRadius;  // formerly "_r"; 
    };



    BHPCircle createEdgeCircle(unsigned int schlafliSymbolP, unsigned int schlafliSymbolQ);



    class AffineTransform
    {

    public:

        AffineTransform();


        BHPPoint transform(BHPPoint somePoint);


        void translate(double x, double y);


        void rotate(double theta);


        void scale(double s);


    //  private:

    public:

        double _a00; double _a01; double _a02;

        double _a10; double _a11; double _a12;
    };






    class FundamentalRegion
    {

    public:

        FundamentalRegion(unsigned int schlafliSymbolP, unsigned int schlafliSymbolQ);  



        bool insideFDT(BHPPoint somePoint);


        bool reversePixelLookup(BHPPoint a, int maxIterations, BHPPoint *out, bool *parity);



    // private:

    public:

        BHPCircle _geodesicEdge;


        double _mSlope;  // tan(PI/p) - slope of line at PI/p

        double _c;	// cos(PI/p)

        double _s;  // sin(PI/P)

        unsigned int _schlafli_p;
    };





    //  Layout:                                  
    //  Layout:    cbuffer conBufMask : register(b0)
    //  Layout:    {
    //  Layout:         float4      AffineABC_; //  _a00, _a01, _a02, * ;
    //  Layout:         float4      AffineDEF_; //  _a10, _a11, _a12, * ; 
    //  Layout:         float4      FDT;        //  slope, radius, cx, cy; 
    //  Layout:    };
    //  Layout:                                  


    struct VHG_ConBufMask
    {
        DirectX::XMFLOAT4       AffineABC_; //  _a00, _a01, _a02, * ;
        DirectX::XMFLOAT4       AffineDEF_; //  _a10, _a11, _a12, * ; 
        DirectX::XMFLOAT4       FDT;        //  slope, radius, cx, cy; 
    };




    struct VHG_ConBufTess
    {
        DirectX::XMFLOAT4       hyp2in_ABC_; //  _a00, _a01, _a02, * ;
        DirectX::XMFLOAT4       hyp2in_DEF_; //  _a10, _a11, _a12, * ; 
        DirectX::XMFLOAT4       out2hyp_ABC_; //  _a00, _a01, _a02, * ;
        DirectX::XMFLOAT4       out2hyp_DEF_; //  _a10, _a11, _a12, * ; 
        DirectX::XMFLOAT4       FDT;        //  slope, radius, cx, cy; 
        DirectX::XMUINT2        SchlafliSymbol;   //  x stores p,  y stores q;
       
        DirectX::XMUINT2        LoopIndex;   //  x stores p,  y stores q;

		DirectX::XMUINT4		xu4a; // e_ShowParity;
    };







    struct VHG_CubeFaceVertexPT
    {
        DirectX::XMFLOAT3       e_pos;
        DirectX::XMFLOAT2       e_texco;
    };










    class Hvy3DScene
    {
    public:

        Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        



        static std::random_device                               e_rng_seed;
        static std::mt19937                                     e_rng_gen;
        static std::uniform_real_distribution<float>            e_rng_dist;








        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();

        void ReleaseDeviceDependentResources();

        void Update(DX::StepTimer const& timer);

        void Render();




    private:

        void        gv_CreateDepthStencilState(); 



        double      gv_GetUserSpecifiedEdgeLength(); 




        void                        gv_FDTMaskRenderPlain(); 

        void                        gv_DispatchComputeShaderFDTMask(); 

        void                        gv_DispatchComputeShaderTessellation(); 




        void                        debouncer_for_keyboard(void);


        void                        gvAnim_AnimationTick();

        bool                        gvAnim_CollisionImminent();


        void                        gvAnim_JumpStep(uint8_t pBits);


        void                        gvAnim_StepLeft();
        void                        gvAnim_StepRight();
        void                        gvAnim_StepUp();
        void                        gvAnim_StepDown();


        void                        gvAnim_JumpLeft();
        void                        gvAnim_JumpRight();
        void                        gvAnim_JumpUp();
        void                        gvAnim_JumpDown();





        void                        gvInitializeSchlafliSymbol(unsigned int SchlafliP, unsigned int SchlafliQ);



        void                        gv_CreateDualViewports();



        void                        gv_CreateBlendState();





        void                        gvConbuf_PrepareConbufData_Mask();


        void                        gvConbuf_PrepareConbufData_Tessellation(unsigned int loopIndex_X, unsigned int loopIndex_Y);




        void                        gv_CreateVertexBuffer_TextureCube();


        void                        gv_CreateVertexBuffer_MonoQuad();


        void                        GoFileSavePicker();



        void                        GoFileOpenPicker();

        Concurrency::task<Windows::Foundation::Size> gvAsync2_ReadFile();

        Windows::Foundation::Size gvAsync3_ReadStream( Windows::Storage::Streams::IBuffer^   fileBuffer );

        Concurrency::task<void> gvAsync_LaunchFileOpenPicker();






        Concurrency::task<UINT> gvAsync_GetImageFromAssetFile(std::wstring const&  relativeFileName);



        Windows::Foundation::Size gvCreateInputResourceView(); // creates resource view on the inputImage;



        Windows::Foundation::Size gvCreateOutputResourceViewsForFDTMask();  //  creates two resource views on the outputImage;

        Windows::Foundation::Size gvCreateOutputResourceViewsForTess();  //  creates two resource views on the outputImage;






    private:


        std::shared_ptr<DX::DeviceResources> m_deviceResources;




        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;



        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_MonoQuadVertexBuffer;
        uint32_t                                                        m_MonoQuadVertexCount;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_MonoQuadIndexBuffer;
        uint32_t                                                        m_MonoQuadIndexCount;


        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_TextureCubeVertexBuffer;
        uint32_t                                                        m_TextureCubeVertexCount;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_TextureCubeIndexBuffer;
        uint32_t                                                        m_TextureCubeIndexCount;





        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader;



        std::unique_ptr<DirectX::Keyboard>                              m_keyboard;
        std::unique_ptr<DirectX::Mouse>                                 m_mouse;
        bool                                                            g0_debouncer_processed_keyboard;

        Windows::Storage::AccessCache::StorageItemAccessList^           e_FAL_list;
        Platform::String^                                               e_chosen_file_name;
        Platform::String^                                               e_chosen_file_path;
        Windows::Storage::StorageFile^                                  e_chosen_file_storage_file;
        unsigned long                                                   e_gnudata_hwm_altitude;
        bool                                                            e_file_picker_complete;
        bool                                                            e_read_gnudata_complete;

        unsigned char *                                                 e_bitmapBuffer;
        uint32_t                                                        e_bitmapNumBytes;


        Microsoft::WRL::ComPtr<ID3D11SamplerState>                      e_SamplerState;

        // int                                                             e_ThreadGroupSizeX;
        // int                                                             e_ThreadGroupSizeY;

        // int                                                             e_BufferWidth;
        // int                                                             e_BufferHeight;




        Microsoft::WRL::ComPtr<ID3D11ComputeShader>                     e_ComputeShaderFDTMask;
        Microsoft::WRL::ComPtr<ID3D11ComputeShader>                     e_ComputeShaderTess;




        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                input_READONLY_SRV;


        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>               output_MASK_as_UAV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                output_MASK_as_SRV;


        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>               output_TESS_as_UAV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                output_TESS_as_SRV;


        Microsoft::WRL::ComPtr<ID3D11Resource>                          output_D3D11_Resource;






        UINT                                                    e_ImageAssetFileSize; // meaning the count of bytes in the file;
        Windows::Foundation::Size                               e_InputTextureSize;  // meaning width in pixels x height in pixels;




        VHG_NOPO                                                e_PositionNOPO;

        VHG_NOPO                                                e_VelocityNOPO;



        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufMaskBuffer;
        VHG_ConBufMask                                          e_ConBufMaskData;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufTessBuffer;
        VHG_ConBufTess                                          e_ConBufTessData;





        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufMaskWVPBuffer;
        ModelViewProjectionConstantBuffer                       e_ConBufMaskWVPData;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufTessWVPBuffer;
        ModelViewProjectionConstantBuffer                       e_ConBufTessWVPData;





        unsigned int                                            schlafli_p;
        unsigned int                                            schlafli_q; 
        BHPCircle                                               e_EdgeCircle;
        FundamentalRegion *                                     e_FDT;


        int                                                             renderedSize;
        int                                                             borderSize;
        double                                                          perturbationFactor;

        int                                                             supersamplingRate;
        int                                                             maxIterations;
        
		unsigned int                                            e_ShowParity;


        bool                                                    e_auto_roving;


        bool    eInvalidated;



        D3D11_VIEWPORT                                          e_ViewportMajor;
        D3D11_VIEWPORT                                          e_ViewportMinor;


        Microsoft::WRL::ComPtr<ID3D11BlendState>                e_BlendState;


        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> ghv_DepthStencilState;  //  <---- here!!!!






        bool    m_loadingComplete;


        float   m_degreesPerSecond;
    };
}


