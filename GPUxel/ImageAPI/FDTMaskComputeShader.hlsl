//      
//      
//      ghv 20190409:  One of the Compute Shaders for real-time hyperbolic image editing;
//      
//      Fundamental Domain Triangle (FDT) Mask Compute Shader;
//      


Texture2D               input_READONLY  : register(t0);

RWTexture2D<float4>     output_as_uav   : register(u0);



cbuffer conBufFDTMask : register(b0)
{
    float4      AffineABC_; //  _a00, _a01, _a02, * ;
    float4      AffineDEF_; //  _a10, _a11, _a12, * ; 
    float4      FDT;        //  slope, radius, cx, cy; 
};



static const float4 maskColor = float4(1.0, 0.0, 0.0, 0.7); // Use 0.5 for alpha blending; 




double2 AffineTransform(double2 somePoint)
{
    double _a00 = AffineABC_.x; 
    double _a01 = AffineABC_.y; 
    double _a02 = AffineABC_.z; 

    double _a10 = AffineDEF_.x; 
    double _a11 = AffineDEF_.y; 
    double _a12 = AffineDEF_.z; 

    double2 otherPoint;

    otherPoint.x = _a00 * somePoint.x + _a01 * somePoint.y + _a02;

    otherPoint.y = _a10 * somePoint.x + _a11 * somePoint.y + _a12;

    return otherPoint;
}





    
bool FDTContainsPoint(double2 somePoint)
{
    bool retval = false; 

    double FDT_mSlope = FDT.x; // TODO:  float-vs-double mismatch;
    double FDT_geodesicEdgeRadius = FDT.y; 
    double FDT_geodesicEdgeCenterX = FDT.z; 
    double FDT_geodesicEdgeCenterY = FDT.w; 


    double gtestx = FDT_geodesicEdgeCenterX - somePoint.x;

    double gtesty = FDT_geodesicEdgeCenterY - somePoint.y;



    if (somePoint.y > FDT_mSlope * somePoint.x)
    {
        retval = false;
    }
    else if (somePoint.y < 0.0)
    {
        retval = false;
    }
    else if (gtestx * gtestx + gtesty * gtesty <= FDT_geodesicEdgeRadius * FDT_geodesicEdgeRadius) // use <= ; 
    {
        retval = false;
    }
    else
    {
        retval = true;
    }

    return retval;
}











float4 negateRGB(float4 somePixel)
{
    return float4(
        1.0 - somePixel.x,
        1.0 - somePixel.y,
        1.0 - somePixel.z,
        somePixel.w
    );
}












[numthreads(10, 10, 1)]
void cs_main(uint3 DTid : SV_DispatchThreadID)
{
    float4 zzzTexel = input_READONLY.Load(int3(DTid.xy, 0)); 


    //   $    
    //   $  To get position of Reticle Grid as either float or unsigned int:
    //   $            
    //   $  float2 posNOPO = float2(conbuf_F2.x, conbuf_F2.y);  
    //   $  or           
    //   $  uint xi = conbuf_UI2.x; uint yi = conbuf_UI2.y;
    //   $    


    double2 inputPoint = double2((double)DTid.x, (double)(DTid.y));

    double2 affinatedPoint = AffineTransform(inputPoint);

    //  test affinated point: is it inside the FDT region ???

    bool  affPointIsInside = FDTContainsPoint( affinatedPoint );  


    if (affPointIsInside == true)
    {
        //  Point is inside the FDT triangle: 

        output_as_uav[DTid.xy] = maskColor; 

        //  output_as_uav[DTid.xy] = negateRGB( float4(zzzTexel.x, zzzTexel.y, zzzTexel.z, 1.0) );
    }
    else
    {
        output_as_uav[DTid.xy] = float4(zzzTexel.x, zzzTexel.y, zzzTexel.z, 1.0);
    }
}




