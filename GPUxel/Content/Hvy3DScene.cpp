

#include "pch.h"

#include "Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace GPUxel;

using namespace DirectX;
using namespace Windows::Foundation;




std::random_device                              Hvy3DScene::e_rng_seed;
std::mt19937                                    Hvy3DScene::e_rng_gen(Hvy3DScene::e_rng_seed());
std::uniform_real_distribution<float>           Hvy3DScene::e_rng_dist(+0.5f, +2.f);;





uint8_t     e_sj_left = 0x0001; 
uint8_t     e_sj_right = 0x0002; 
uint8_t     e_sj_up = 0x0004; 
uint8_t     e_sj_down = 0x0008; 
uint8_t     e_sj_step = 0x0010; 
uint8_t     e_sj_jump = 0x0020; 




        
double  Hvy3DScene::gv_GetUserSpecifiedEdgeLength()
{

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
    //   
    //  originally was 150.00 when using 300 pixel inputImage;
    //      


    double edgeLength = 1024.00 / 2.00;

    return edgeLength;
}
















BHPCircle::BHPCircle()
{
    _circleRadius = 1.0;

    _circleCenter.x = 0.0;

    _circleCenter.y = 0.0;
}







BHPCircle::BHPCircle(BHPPoint circleCenter, double circleRadius)
{
    _circleRadius = circleRadius;

    _circleCenter = circleCenter;
}







bool BHPCircle::insideOrOn(BHPPoint somePoint)
{
    double x = _circleCenter.x - somePoint.x;

    double y = _circleCenter.y - somePoint.y;

    return (x * x + y * y <= _circleRadius * _circleRadius); 
}







BHPPoint BHPCircle::invert(BHPPoint somePoint)
{
    double dx = somePoint.x - _circleCenter.x;

    double dy = somePoint.y - _circleCenter.y;

    double rSquared = dx * dx + dy * dy;





    double r2 = _circleRadius * _circleRadius / rSquared;


    double x2 = r2 * dx;

    double y2 = r2 * dy;


    BHPPoint q;

    q.x = _circleCenter.x + x2;

    q.y = _circleCenter.y + y2;


    return q;
}










BHPCircle GPUxel::createEdgeCircle(unsigned int schlafliSymbolP, unsigned int schlafliSymbolQ) 
{
    double cosq2 = cos(PI / ((double)schlafliSymbolQ))*cos(PI / ((double)schlafliSymbolQ));

    double sinp2 = sin(PI / ((double)schlafliSymbolP))*sin(PI / ((double)schlafliSymbolP));


    BHPPoint cCenter;
    cCenter.x = sqrt(cosq2 / (cosq2 - sinp2));
    cCenter.y = 0.0;


    double cRadius = sqrt(sinp2 / (cosq2 - sinp2));

    return BHPCircle(cCenter, cRadius);
}








AffineTransform::AffineTransform()
{
    _a00 = 1.0;     _a01 = 0.0;     _a02 = 0.0;

    _a10 = 0.0;     _a11 = 1.0;     _a12 = 0.0;
}





BHPPoint AffineTransform::transform(BHPPoint somePoint)
{
    BHPPoint q;

    q.x = _a00 * somePoint.x + _a01 * somePoint.y + _a02;

    q.y = _a10 * somePoint.x + _a11 * somePoint.y + _a12;

    return q;
}





void AffineTransform::translate(double x, double y)
{
	_a02 -= x;

	_a12 -= y;
}








void AffineTransform::rotate(double theta)
{
	double a00 = _a00 * cos(theta) - _a10 * sin(theta);
	double a01 = _a01 * cos(theta) - _a11 * sin(theta);
	double a02 = _a02 * cos(theta) - _a12 * sin(theta);

	double a10 = _a00 * sin(theta) + _a10 * cos(theta);
	double a11 = _a01 * sin(theta) + _a11 * cos(theta);
	double a12 = _a02 * sin(theta) + _a12 * cos(theta);

	_a00 = a00;
	_a01 = a01;
	_a02 = a02;
	_a10 = a10;
	_a11 = a11;
	_a12 = a12;
}





void AffineTransform::scale(double s)
{
    _a00 *= s;
    _a01 *= s;
    _a02 *= s;
    _a10 *= s;
    _a11 *= s;
    _a12 *= s;
}








FundamentalRegion::FundamentalRegion(unsigned int schlafliSymbolP, unsigned int schlafliSymbolQ)
{
    _schlafli_p = schlafliSymbolP;


    _geodesicEdge = createEdgeCircle(schlafliSymbolP, schlafliSymbolQ);



    _mSlope = tan(PI / (double)schlafliSymbolP);


    _c = cos(2.0 * PI / (double)schlafliSymbolP);


    _s = sin(2.0 * PI / (double)schlafliSymbolP);
}








bool FundamentalRegion::insideFDT(BHPPoint somePoint)
{
    if (somePoint.y > _mSlope * somePoint.x) return false;

    if (somePoint.y < 0.0) return false;

    if (_geodesicEdge.insideOrOn(somePoint)) return false;


    return true;
}








// This is the reverse pixel lookup routing.  Parity is useful
// for doing things like creating tessellation diagrams where
// alternating triangles are different colors.


bool FundamentalRegion::reversePixelLookup(BHPPoint a, int maxIterations, BHPPoint *out, bool *parity)
{
    int d = maxIterations;

    BHPPoint b = a;


    while (d >= 0)
    {


#if 0

        // Note:  this is more efficient than the below code, but
        // it can get into an infinite loop, which appears to be
        // due to some kind of numerical precision problem.

        // rotate point to within +/- PI/p
        // is it within the cone defined by +/- _m?
        BHPPoint c = b;
        while ((c.y > _m * c.x) || (c.y < -_m * c.x)) {
            // apply a linear rotation transformation
            double tmp = _c * c.x - _s * c.y;
            c.y = _s * c.x + _c * c.y;
            c.x = tmp;
        }

#else

        //  
        // rotate point to within +/- PI/p
        //  
        // TODO:  Make this more efficient!
        //  

        double theta = atan2(b.y, b.x);


        double r = sqrt(b.x * b.x + b.y * b.y);


        while (theta < 0.0) theta += 2.0 * PI;



        while (theta > PI / (double)_schlafli_p) 
        {
            theta -= 2.0 * PI / (double)_schlafli_p;
        }




        BHPPoint c;

        c.x = r * cos(theta);
        c.y = r * sin(theta);

#endif


        if (c.y < 0.0) 
        {
            c.y = -c.y;

            *parity = (*parity) ? false : true;
        }


        if (insideFDT(c)) 
        {
            out->x = c.x;
            out->y = c.y;
            return false;
        }



        b = _geodesicEdge.invert(c);


        *parity = (*parity) ? false : true;


        if (insideFDT(b)) 
        {
            out->x = b.x;
            out->y = b.y;
            return false;
        }

        d--;
    }

    return true;
}










        
void Hvy3DScene::gvInitializeSchlafliSymbol(unsigned int SchlafliP, unsigned int SchlafliQ)
{

    if (e_FDT != nullptr)
    {
        delete e_FDT; // TODO; class dtor...

        e_FDT = nullptr;
    }



    this->schlafli_p = SchlafliP; 
    this->schlafli_q = SchlafliQ; 


    if ((schlafli_p - 2) * (schlafli_q - 2) <= 4) 
    {
        throw "(p-2)(q-2) must be greater than 4";
    }


    e_EdgeCircle = createEdgeCircle(schlafli_p, schlafli_q);  //  TODO: calculate every time the user changes Schlafli Symbol;



    e_FDT = new FundamentalRegion(schlafli_p, schlafli_q);  // TODO: free heap allocated memory; 

}









Hvy3DScene::Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources) :

	m_loadingComplete(false),

	m_degreesPerSecond(45),



	m_TextureCubeIndexCount(0),
	m_MonoQuadIndexCount(0),




    g0_debouncer_processed_keyboard(false),

    e_auto_roving(false), 



    e_PositionNOPO(0.01333f, 0.03777f), 

    e_VelocityNOPO(0.5f, 0.5f), 



    //  e_ThreadGroupSizeX(16), e_ThreadGroupSizeY(16),




    renderedSize(2048),  // revert to renderedSize = 900 or 1200; 


    borderSize(4),


    supersamplingRate(8),


    perturbationFactor(0.00),
    maxIterations(16),  // revert to 20;



    e_ShowParity(1),




    eInvalidated(true), 






	m_deviceResources(deviceResources)
{
    // Tessellation {5, 5} tested good:  
    
    gvInitializeSchlafliSymbol(5, 5);



    //  good for Alex' FamilyTree input image:     gvInitializeSchlafliSymbol(4, 5);


    // Tessellation {10, 5} tested good:   gvInitializeSchlafliSymbol(10, 5);






    //  tested good:  perfect for guidance logo heads:   gvInitializeSchlafliSymbol(3, 14);


    //  tested ok    gvInitializeSchlafliSymbol(7, 3);



	CreateDeviceDependentResources();


	CreateWindowSizeDependentResources();


    m_keyboard = std::make_unique<DirectX::Keyboard>();
    m_mouse = std::make_unique<DirectX::Mouse>();


    Windows::UI::Core::CoreWindow  ^better_core_win = Windows::UI::Core::CoreWindow::GetForCurrentThread();

    m_keyboard->SetWindow(better_core_win);
    m_mouse->SetWindow(better_core_win);




}






























// Initializes view parameters when the window size changes.

void Hvy3DScene::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;






	//   
    //  not  if (aspectRatio < 1.0f) { fovAngleY *= 2.0f; }
    //          



	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.







	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(   //  left-handed chirality;
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);



	XMStoreFloat4x4( &e_ConBufMaskWVPData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix) );

	XMStoreFloat4x4( &e_ConBufTessWVPData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix) );







#ifdef GHV_OPTION_MESH_MONOQUAD   //  choice between a Texture Cube and a Mono Quad; 

	static const XMVECTORF32 eye = { 0.0f, 0.0f, -4.5f, 0.0f };  //  cameraPosition:  use negative z to pull camera back;

	static const XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };

#else

	static const XMVECTORF32 eye = { 0.0f, 2.7f, -4.5f, 0.0f };  //  cameraPosition:  use negative z to pull camera back;

	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
#endif





	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };


	XMStoreFloat4x4(&e_ConBufMaskWVPData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up))); // left-handed chirality;

	XMStoreFloat4x4(&e_ConBufTessWVPData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up))); // left-handed chirality;

}

























void Hvy3DScene::debouncer_for_keyboard(void)
{
    static uint32_t idx_waiting = 0;
    if (g0_debouncer_processed_keyboard)
    {
        idx_waiting++;
        if (idx_waiting > 10)  //  use period = 30; 
        {
            idx_waiting = 0;
            g0_debouncer_processed_keyboard = false;
        }
    }
}











        
bool Hvy3DScene::gvAnim_CollisionImminent()
{
    const float epsilon = 0.45f; // gold = 0.35f;

    bool retval = false;

    //  Domain: 

    /*
    float domainXmin = -0.5f; 
    float domainXMax = +0.25f;
    float domainYmin = -0.5f;
    float domainYMax = +0.5f;
    */

    float domainXmin = -1.0f; 
    float domainXMax = +1.0f;
    float domainYmin = -1.0f;
    float domainYMax = +1.0f;


    bool hLeft = (std::abs(e_PositionNOPO.x - domainXmin) < epsilon);
    bool hRight = (std::abs(e_PositionNOPO.x - domainXMax) < epsilon);

    bool vTop = (std::abs(e_PositionNOPO.y - domainYmin) < epsilon);
    bool vBottom = (std::abs(e_PositionNOPO.y - domainYMax) < epsilon);

    float aabs = 0.f;
    float aLimited = 0.f;


    if ( (hLeft && vTop) || (hLeft && vBottom) || (hRight && vTop)  || (hRight && vBottom) )
    {
        //  corner cases, literally: reflect both components of velocity: 

        e_VelocityNOPO.x *= -1.f;
        e_VelocityNOPO.y *= -1.f;
    }
    else if (hLeft || hRight)
    {
        // reflect horizontal velocity: 

        e_VelocityNOPO.x *= -1.f;
    }
    else if (vTop || vBottom)
    {
        // reflect vertical velocity: 

        e_VelocityNOPO.y *= -1.f;
    }
    else
    {
        // no collision is imminent, so take the opportunity
        // to randomize the velocity

        e_VelocityNOPO.x *= std::abs(e_rng_dist(e_rng_gen));
        aabs = std::abs(e_VelocityNOPO.x);
        aLimited = (std::min)(aabs, 1.6f);
        e_VelocityNOPO.x = copysign(aLimited, e_VelocityNOPO.x);



        e_VelocityNOPO.y *= std::abs(e_rng_dist(e_rng_gen));
        aabs = std::abs(e_VelocityNOPO.y);
        aLimited = (std::min)(aabs, 1.6f);
        e_VelocityNOPO.y = copysign(aLimited, e_VelocityNOPO.y);
    }

    //  
    //  Now that the velocity's direction has been corrected, 
    //  can scale the velocity's magnitude as long as signum is unchanged
    //  

    return retval;
}







void Hvy3DScene::gvAnim_AnimationTick()
{
    //  gold:   const float xyStepDistance = 0.0545454;   //  proportional to speed;


    //  excellent:  const float xyStepDistance = 0.0345345; 


    const float xyStepDistance = 0.024f; 


    if (e_auto_roving == true)
    {
        //  Causes the Reticle Grid to move randomly like a Pong arcade game;

        gvAnim_CollisionImminent();

        e_PositionNOPO.x += e_VelocityNOPO.x * xyStepDistance;

        e_PositionNOPO.y += e_VelocityNOPO.y * xyStepDistance;

        eInvalidated = true;
    }

}





void Hvy3DScene::gvAnim_JumpStep(uint8_t pBits)
{
    const float xyStepDistance = 0.024f; 
    const float xyJumpDistance = 0.12f; 

    float dist = 0.f;
    bool valid = false;

    if ((pBits & e_sj_step) == e_sj_step)
    {
        dist = xyStepDistance;
    }
    else if ((pBits & e_sj_jump) == e_sj_jump)
    {
        dist = xyJumpDistance;
    }



    if ((pBits & e_sj_left) == e_sj_left)
    {
        e_PositionNOPO.x -= dist;
        valid = true;
    }
    else if ((pBits & e_sj_right) == e_sj_right)
    {
        e_PositionNOPO.x += dist;
        valid = true;
    }
    else if ((pBits & e_sj_up) == e_sj_up)
    {
        e_PositionNOPO.y -= dist;
        valid = true;
    }
    else if ((pBits & e_sj_down) == e_sj_down)
    {
        e_PositionNOPO.y += dist;
        valid = true;
    }



    if (valid) 
    {
        eInvalidated = true;
    }
}





void Hvy3DScene::gvAnim_StepLeft()
{
    // const float xyStepDistance = 0.024f; e_PositionNOPO.x -= xyStepDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_step | e_sj_left); 
}

void Hvy3DScene::gvAnim_StepRight()
{
    // const float xyStepDistance = 0.024f; e_PositionNOPO.x += xyStepDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_step | e_sj_right); 
}

void Hvy3DScene::gvAnim_StepUp()
{
    // const float xyStepDistance = 0.024f; e_PositionNOPO.y -= xyStepDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_step | e_sj_up); 
}

void Hvy3DScene::gvAnim_StepDown()
{
    // const float xyStepDistance = 0.024f; e_PositionNOPO.y += xyStepDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_step | e_sj_down); 
}










void Hvy3DScene::gvAnim_JumpLeft()
{
    // const float xyJumpDistance = 0.12f; e_PositionNOPO.x -= xyJumpDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_jump | e_sj_left); 
}


void Hvy3DScene::gvAnim_JumpRight()
{
    // const float xyJumpDistance = 0.12f; e_PositionNOPO.x += xyJumpDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_jump | e_sj_right); 
}


void Hvy3DScene::gvAnim_JumpUp()
{
    // const float xyJumpDistance = 0.12f; e_PositionNOPO.y -= xyJumpDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_jump | e_sj_up); 
}


void Hvy3DScene::gvAnim_JumpDown()
{
    // const float xyJumpDistance = 0.12f; e_PositionNOPO.y += xyJumpDistance; eInvalidated = true;

    gvAnim_JumpStep(e_sj_jump | e_sj_down); 
}







void Hvy3DScene::Update(DX::StepTimer const& timer)
{
    DirectX::Keyboard::State           kb = m_keyboard->GetState();



    if (kb.O)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;

            e_auto_roving = false;
            eInvalidated = false; // halt CS Compute Shader invocation;

            GoFileOpenPicker();
        }
    }



    if (kb.S)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;

            GoFileSavePicker();
        }
    }





    if (kb.I)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;

			e_ShowParity = (e_ShowParity) ? 0 : 1;
        }
    }








    if (kb.F3)
    {

        if (e_auto_roving == true)
        {
            e_auto_roving = false;
        
            eInvalidated = false;
        }
    } 



    if (kb.F4)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;

            eInvalidated = true;

            if (e_auto_roving == false)
            {
                e_auto_roving = true;
            }
        }
    } 



    if(kb.Left)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepLeft();
        }
    } 

    if(kb.A)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpLeft();
        }
    } 








    if(kb.Right)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepRight();
        }
    } 

    if(kb.D)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpRight();
        }
    } 






    if(kb.Up)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepUp();
        }
    } 

    if(kb.W)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpUp();
        }
    } 








    if(kb.Down)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepDown();
        }
    } 

    if(kb.X)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpDown();
        }
    } 









    //  const float framePeriod = 2.f;  // Animation speed: Was too fast at 0.10f;  Use from 0.20f to 0.9 seconds per frame;


    //  const float framePeriod = 1.f;  // Animation speed: framePeriod = 2.f is gold; 


    const float framePeriod = 0.666f;  // Animation speed: framePeriod = 0.666 sec to be used with 30 frames per second rendering; 


    static float accumTime = 0.f; 
    accumTime += (float)timer.GetElapsedSeconds();
    if (accumTime > framePeriod)
    {
        gvAnim_AnimationTick(); 
        accumTime = 0.f;
    }






    float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
	double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
    float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

    XMMATRIX maskRot = XMMatrixRotationY(radians); // the Mask is on the rotating cube;

    // override: 
    maskRot = XMMatrixIdentity(); 

    XMMATRIX maskXlat = XMMatrixTranslation(-3.f, 0.f, 0.f); // the Mask is on the rotating cube;


#ifdef GHV_OPTION_DUAL_VIEWPORTS

    maskXlat = XMMatrixIdentity();

#endif


    XMMATRIX maskWorldXform = maskRot * maskXlat; //  First rotate the cube about the world origin, then translate the rotated cube from origin to destination;

    XMStoreFloat4x4(&e_ConBufMaskWVPData.model, XMMatrixTranspose(maskWorldXform));



    XMStoreFloat4x4(&e_ConBufTessWVPData.model, XMMatrixTranspose(XMMatrixIdentity())); // don't rotate the Tessellation on the MonoQuad;

}






















void Hvy3DScene::Render()
{
	if (!m_loadingComplete)
	{
		return; // Loading is asynchronous. Only draw geometry after it's loaded.
	}


    this->debouncer_for_keyboard(); 




   gv_FDTMaskRenderPlain(); 
	
    auto context = m_deviceResources->GetD3DDeviceContext();
    
    context->OMSetBlendState(e_BlendState.Get(), NULL, 0xFFFFFFFF);  //  keywords: blendstate, alpha, blending;

    context->OMSetDepthStencilState(ghv_DepthStencilState.Get(), 0);





    gv_DispatchComputeShaderFDTMask(); 


    gv_DispatchComputeShaderTessellation(); 

}





















void Hvy3DScene::GoFileOpenPicker()
{
    this->eInvalidated = true;  // TODO:  remove this. logic is wrong.



    e_gnudata_hwm_altitude = 0; // Serves as an "invalid" flag; 



    this->gvAsync_LaunchFileOpenPicker().then([this]()
    {
    
            if (this->e_chosen_file_storage_file)
            {
     
                this->e_file_picker_complete = true;


                gvAsync2_ReadFile().then([this](Windows::Foundation::Size returnedSize)
                {
                        // gvCreateOutputResourceViewsForFDTMask();  

                        // gvCreateOutputResourceViewsForTess();  

                        e_read_gnudata_complete = true;
                });
            }
    });
}


















void Hvy3DScene::CreateDeviceDependentResources()
{


#ifdef GHV_OPTION_DUAL_VIEWPORTS

    gv_CreateDualViewports();  

#endif




    gv_CreateDepthStencilState(); 


    gv_CreateBlendState();  






    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


    //  Sampler State Object for the Pixel Shader to render the Bitmap: 

    {
        D3D11_SAMPLER_DESC sampDescZ;
        ZeroMemory(&sampDescZ, sizeof(sampDescZ));
        sampDescZ.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;


        //  To render the bitmap via pixel shader use D3D11_TEXTURE_ADDRESS_CLAMP:

        sampDescZ.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDescZ.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDescZ.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;



        // sampDescZ.AddressU = sampDescZ.AddressV = sampDescZ.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; // undo;



        sampDescZ.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDescZ.MinLOD = 0;
        sampDescZ.MaxLOD = D3D11_FLOAT32_MAX;

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateSamplerState(
                &sampDescZ,
                e_SamplerState.ReleaseAndGetAddressOf()
            )
        );
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%










    
    // Load shaders asynchronously.

	auto loadVSTask = DX::ReadDataAsync(L"t1VertexShader.cso");

	auto loadPSTask = DX::ReadDataAsync(L"t1PixelShader.cso");

	auto loadCSMaskTask = DX::ReadDataAsync(L"FDTMaskComputeShader.cso");

	auto loadCSTessTask = DX::ReadDataAsync(L"TessellationComputeShader.cso");



    gvAsync_GetImageFromAssetFile(L"\\Assets\\0_Input_Image_1024.png").then([this](UINT actualFileSize) {
        
        this->e_ImageAssetFileSize = actualFileSize;
        
    });


    gvCreateOutputResourceViewsForFDTMask();  


    gvCreateOutputResourceViewsForTess();  








    auto createCSMaskTask = loadCSMaskTask.then([this](const std::vector<byte>& fileData)
    {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&e_ComputeShaderFDTMask
				)
			);
    });





    auto createCSTessTask = loadCSTessTask.then([this](const std::vector<byte>& fileData)
    {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&e_ComputeShaderTess
				)
			);
    });






















	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);


		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
		{
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 }, // float3 for VHG_CubeFaceVertexPT.e_pos;
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 }  // float2 for VHG_CubeFaceVertexPT.e_texco;
		};



		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});












	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);


		CD3D11_BUFFER_DESC constantBufferWVPDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);

        static_assert( (sizeof(ModelViewProjectionConstantBuffer) % 16) == 0, "Constant Buffer struct must be 16-byte aligned" );

		DX::ThrowIfFailed( m_deviceResources->GetD3DDevice()->CreateBuffer( &constantBufferWVPDesc, nullptr, &e_ConBufMaskWVPBuffer ) );
		DX::ThrowIfFailed( m_deviceResources->GetD3DDevice()->CreateBuffer( &constantBufferWVPDesc, nullptr, &e_ConBufTessWVPBuffer ) );






		CD3D11_BUFFER_DESC constantBufferMaskDesc(sizeof(VHG_ConBufMask) , D3D11_BIND_CONSTANT_BUFFER);
        static_assert( (sizeof(VHG_ConBufMask) % 16) == 0, "Constant Buffer struct must be 16-byte aligned" );
		DX::ThrowIfFailed( m_deviceResources->GetD3DDevice()->CreateBuffer( &constantBufferMaskDesc, nullptr, &e_ConBufMaskBuffer ) );




        
		CD3D11_BUFFER_DESC constantBufferTessDesc(sizeof(VHG_ConBufTess) , D3D11_BIND_CONSTANT_BUFFER);
        static_assert( (sizeof(VHG_ConBufTess) % 16) == 0, "Constant Buffer struct must be 16-byte aligned" );
		DX::ThrowIfFailed( m_deviceResources->GetD3DDevice()->CreateBuffer( &constantBufferTessDesc, nullptr, &e_ConBufTessBuffer ) );
        
    });










	auto createCubeTask = (createCSMaskTask && createCSTessTask && createPSTask && createVSTask).then([this] () 
    {
		// Load mesh vertices.

        gv_CreateVertexBuffer_MonoQuad(); 

        gv_CreateVertexBuffer_TextureCube(); 
	});






	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}


















void Hvy3DScene::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();


	e_ConBufMaskWVPBuffer.Reset();
	e_ConBufTessWVPBuffer.Reset();


	m_TextureCubeVertexBuffer.Reset();
	m_MonoQuadVertexBuffer.Reset();

	m_TextureCubeIndexBuffer.Reset();
	m_MonoQuadIndexBuffer.Reset();
}


















void Hvy3DScene::gv_CreateVertexBuffer_MonoQuad()
{
    //  texco texture coordinates will be set inside the VS vertex shader. 

    // ???  increasing negative values of z push the mesh deeper into the background: 


    //  
    //  ghv 20190410: I altered z_coordinate of the MonoQuad mesh vertices
    //  because it improved render size of the Poincare Disk. 
    //  When z_coord = -1.f, the Poincare Disk is rendered very small on the screen.
    //  But using z_coord = -3.f gives a Poincare Disk large enough to fill the screen.
    //  

    const float z_coord = -3.f; //  Use z_coord = -3.f to render large Poincare Disk; 


    std::vector<VHG_CubeFaceVertexPT> monoQuadVertices = 
    {
        {XMFLOAT3(-1.0f, -1.0f,  z_coord), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(+1.0f, -1.0f,  z_coord), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, +1.0f,  z_coord), XMFLOAT2(0.0f, 0.0f)},

        {XMFLOAT3(-1.0f, +1.0f,  z_coord), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(+1.0f, -1.0f,  z_coord), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(+1.0f, +1.0f,  z_coord), XMFLOAT2(1.0f, 0.0f)},
    };
    
    m_MonoQuadVertexCount = (uint32_t)monoQuadVertices.size();

    const VHG_CubeFaceVertexPT   *p_vertices = &(monoQuadVertices[0]);

    size_t a_required_allocation = sizeof(VHG_CubeFaceVertexPT) * m_MonoQuadVertexCount;

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = p_vertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)a_required_allocation, D3D11_BIND_VERTEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_MonoQuadVertexBuffer
        )
    );


    //===============================================
    //  Each triangle below is FRONT_FACE_CLOCKWISE: 
    //          
    //  (cf D3D11_RASTERIZER_DESC.FrontCounterClockwise);
    //      
    //  Each trio of index entries represents one triangle. 
    //===============================================

    static const unsigned short quadIndices[] =
    {
        0,2,1,
        5,4,3,
    };

    m_MonoQuadIndexCount = ARRAYSIZE(quadIndices);


    D3D11_SUBRESOURCE_DATA quad_ib_data = { 0 };
    quad_ib_data.pSysMem = quadIndices;
    quad_ib_data.SysMemPitch = 0;
    quad_ib_data.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC quad_ib_descr(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &quad_ib_descr, 
            &quad_ib_data, 
            &m_MonoQuadIndexBuffer
        )
    );
}













void Hvy3DScene::gv_CreateVertexBuffer_TextureCube()
{

    //      
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //      
    //                  Texture Coordinates (u,v)
    //                  =========================
    //      
    //  As an ordered pair, (u,v) can take but four values, i.e. 
    //  (u,v) is an element of the set {00, 10, 11, 01}.
    //      
    //  Arrange these four potential values in a circular fashion: 
    //          
    //                                  01
    //                          
    //                            00          11
    //                  
    //                                  10
    //      
    //      
    //  While traversing the circular arrangement clockwise, the correct 
    //  texture coordinate values for the +y face, the -x face and the-z face
    //  are obtained. 
    //  
    //  Then anticlockwise traversal of circle gives the proper texture 
    //  coordinates for the remaining three faces of the cube. 
    //    
    //  Only two orderings can be said to exist for the four tokens 
    //  arranged in the circle: the clockwise ordering and the anticlockwise ordering. 
    //  The starting position on the circle corresponds to order-preserving 
    //  permutations of the four elements, and also correspond to rotations
    //  of the bitmap image as displayed on the meshed cube. 
    //  Rotating the circular quartet of uv pairs shown above effects 
    //  that rotation of the bitmap image on its face of the meshed cube. 
    //  
    //      
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //      



    //  Take advantage of C++11 braced initialization for std::vector:


    std::vector<VHG_CubeFaceVertexPT> cubeVertices = {


        //  
        //     the +y face: somebody looking at the cube would call this the "top" of the cube;
        //  Correct uv pairs are 01, 11,10, 00 which are the numbers obtained 
        //  by clockwise traversal of the circular diagram. 
        // 
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) },  // A  now correct;
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },  // B  now correct;
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) },  // C  now correct;
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) },  // D  now correct;
        //    
        //     the -y face: the underside, seldom visible face of the cube; 
        //  The circular diagram, traversed ANTICLOCKWISE gives 00, 10, 11, 01: 
        //    
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) }, // A is repaired;
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) }, // B is repaired;
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // C is repaired;
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) }, // D is repaired;





        // 
        //    the  -x face:  a person looking at the cube would call this the "left" face;
        //   
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) },  // now correct;  A;
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },  // now correct;  B; 
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) },  // now correct;  C; 
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) },  // now correct;  D; 
        // 
        //    the +x face: viewer looking at the cube would call this the "right" face;
        //   
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // A (now correct);
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) }, // B (now correct);
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) }, // C (now correct);
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) }, // D (now correct);




        //  
        //   the front face is the -z face: pick uv values by moving clockwise around the circle;
        //  
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) },   //  A  16  okay;
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },   //  B  17  okay;
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) },   //  C  18  okay;
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) },   //  D  19  okay;
        //  
        //      the back face is the +z face: uv values given by moving anticlockwise around the circle;
        //  
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // anticlockwise around the circle;
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) }, // anticlockwise;
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) }, // anticlockwise;
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) }  // anticlockwise;

    };

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


    m_TextureCubeVertexCount = (uint32_t)cubeVertices.size();


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


    const VHG_CubeFaceVertexPT   *p_vertices = &(cubeVertices[0]);


    size_t a_required_allocation = sizeof(VHG_CubeFaceVertexPT) * m_TextureCubeVertexCount;


    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = p_vertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;


    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)a_required_allocation, D3D11_BIND_VERTEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_TextureCubeVertexBuffer
        )
    );


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    //   
    //     Don't try this without the Index Buffer
    // 

    static const unsigned short cubeIndices[] =
    {
        3,1,0,          2,1,3,
        6,4,5,          7,4,6,
        11,9,8,         10,9,11,
        14,12,13,       15,12,14,
        19,17,16,       18,17,19,
        22,20,21,       23,20,22
    };

    m_TextureCubeIndexCount = ARRAYSIZE(cubeIndices);


    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    ZeroMemory(&indexBufferData, sizeof(indexBufferData));
    indexBufferData.pSysMem = cubeIndices;
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc, &indexBufferData, &m_TextureCubeIndexBuffer
        )
    );
}


















void Hvy3DScene::gv_CreateDualViewports()
{
    //  Construct the Major Viewport: 

    float const renderTargetWidth = m_deviceResources->GetOutputSize().Width; 
    float const renderTargetHeight = m_deviceResources->GetOutputSize().Height; 

    e_ViewportMajor = CD3D11_VIEWPORT(
        0.f,                    //  top_leftX;
        0.f,                    //  top_leftY;
        renderTargetWidth,      //  width;
        renderTargetHeight      //  height;
    );


    //  Construct the minor Viewport: 

    float const vpFraction = 0.333f;  //  Use 0.25f or use 0.333f; 



    float const vpAspectRatio = renderTargetWidth / renderTargetHeight;
    float const minorWidth = vpFraction * renderTargetWidth;
    float const minorHeight = minorWidth / vpAspectRatio;

    float top_leftX_viewport_B = renderTargetWidth - minorWidth;
    float top_leftY_viewport_B = renderTargetHeight - minorHeight;

    e_ViewportMinor = CD3D11_VIEWPORT(
        top_leftX_viewport_B,   //  top_leftX;
        top_leftY_viewport_B,   //  top_leftY;
        minorWidth,             //  width;
        minorHeight             //  height;
    );



    /*
    D3D11_VIEWPORT arrViewports[2];
    arrViewports[0] = e_ViewportMajor;
    arrViewports[1] = e_ViewportMinor;
    m_deviceResources->GetD3DDeviceContext->RSSetViewports(2, arrViewports);
    */

}











void Hvy3DScene::gv_CreateDepthStencilState()
{
    //          
 //      ghv : Create Depth-Stencil State: 
 //          

    D3D11_DEPTH_STENCIL_DESC gv_depth_stencil_descr = { 0 };


    // Depth test parameters
    //---------------------------------------------------------------
    gv_depth_stencil_descr.DepthEnable = true;
    gv_depth_stencil_descr.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    gv_depth_stencil_descr.DepthFunc = D3D11_COMPARISON_ALWAYS;  // ghv : to allow blending;

    // Stencil test parameters
    //---------------------------------------------------------------
    gv_depth_stencil_descr.StencilEnable = true;
    gv_depth_stencil_descr.StencilReadMask = 0xFF;
    gv_depth_stencil_descr.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    //---------------------------------------------------------------
    gv_depth_stencil_descr.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    gv_depth_stencil_descr.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    //---------------------------------------------------------------
    gv_depth_stencil_descr.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    gv_depth_stencil_descr.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;


    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateDepthStencilState(
            &gv_depth_stencil_descr,
            ghv_DepthStencilState.GetAddressOf()
        )
    );


    //                  
    //      ghv : Bind the Depth-Stencil State to the OM Stage: 
    //                  

    //  m_d3dContext->OMSetDepthStencilState(ghv_DepthStencilState.Get(), 0);





}







void Hvy3DScene::gv_CreateBlendState()
{


    D3D11_RENDER_TARGET_BLEND_DESC  rt_blend_descr = { 0 };

    rt_blend_descr.BlendEnable = TRUE;

    rt_blend_descr.SrcBlend = D3D11_BLEND_SRC_ALPHA;        // SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt_blend_descr.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt_blend_descr.BlendOp = D3D11_BLEND_OP_ADD;            // BlendOp = D3D11_BLEND_OP_ADD;

    rt_blend_descr.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt_blend_descr.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt_blend_descr.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    rt_blend_descr.RenderTargetWriteMask = 0x0F;





    D3D11_BLEND_DESC  d3d11_blend_descr = { 0 };

    // undo d3d11_blend_descr.AlphaToCoverageEnable = TRUE; // Need AlphaToCoverageEnable = TRUE to make sphere transparent;

    d3d11_blend_descr.AlphaToCoverageEnable = FALSE;





    // This is probably because the DDS image is of a multi-pane glass window fixture,
    // a matrix of glass with alpha = 0 ==> fully transparent 
    // separated by "wooden" cross members...which are opaque as wood often is.




    // d3d11_blend_descr.IndependentBlendEnable = FALSE;

    d3d11_blend_descr.IndependentBlendEnable = TRUE;



    d3d11_blend_descr.RenderTarget[0] = { rt_blend_descr };




    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBlendState(
            &d3d11_blend_descr,
            e_BlendState.GetAddressOf()
        )
    );

}











