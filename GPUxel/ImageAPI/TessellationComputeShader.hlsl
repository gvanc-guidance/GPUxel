//      
//      
//      ghv 20190409:  Compute Shader for real-time hyperbolic image editing;
//      
//      Tessellation Compute Shader for tiling by arbitrary Schlafli Symbol; 
//      


Texture2D               input_READONLY  : register(t0);

RWTexture2D<float4>     output_as_uav   : register(u0);



cbuffer conBufTess : register(b0)
{
    float4      hyp2in_ABC_; //  _a00, _a01, _a02, * ;
    float4      hyp2in_DEF_; //  _a10, _a11, _a12, * ; 

    float4      out2hyp_ABC_; //  _a00, _a01, _a02, * ;
    float4      out2hyp_DEF_; //  _a10, _a11, _a12, * ; 

    float4      FDT;        //  slope, radius, cx, cy; 

    uint2       SchlafliSymbol; // schlafli_p, schlafli_q;
    uint2       LoopIndex;

	uint4		xu4a;  // e_ShowParity stored in .x;
};

    
static const int supersamplingRate = 8;  // hard-coded; 





static const float4 outOfBoundsColor = float4(1.0, 1.0, 1.0, 1.0); // white; 

static const float4 unreachableColor = float4(0.0, 1.0, 0.0, 1.0); // green; 

static const float4 nonSourceColor = float4(0.0, 0.0, 1.0, 1.0); // blue; 

static const float4 maskColor = float4(0.0, 0.0, 0.0, 1.0); 

static const int retFail = -1; 

static const int retOkay = 0; 

static const float mystery = 0.0; //  ? Why is this here ? 

static const double konst_pi = 3.1415926535897932384626433;








//  TODO:  double-precision ???   int GetSubPixelFromInputImage(double xInput, double yInput)


int GetSubPixelFromInputImage(float xInput, float yInput, out float4  outPixel)
{
    int _width = 1024;  // hard-coded width in pixels of the inputImage (typically 300 pixels);

    int _height = 1024;  // hard-coded height in pixels of the inputImage (typically 300 pixels);




    //  Result (to be used in the outputImage) is obtained by 
    //  interpolating around a sub-pixel inside the inputImage. 

    int  x1 = (int)floor(xInput);
    int  x2 = x1 + 1;


    int y1 = (int)floor(yInput);
    int y2 = y1 + 1;


    // pixel out of bounds

    if ((x1 < 0) || (x2 >= _width) || (y1 < 0) || (y2 >= _height))
    {
        return retFail;
    }


    float4 pixel_1 = input_READONLY.Load(int3(x1, y1, 0)); 
    float4 pixel_2 = input_READONLY.Load(int3(x1, y2, 0)); 
    float4 pixel_3 = input_READONLY.Load(int3(x2, y1, 0)); 
    float4 pixel_4 = input_READONLY.Load(int3(x2, y2, 0)); 



    //  Assumption:  pixel_1.x is the red channel, pixel_1.y is green, and pixel_1.z is blue. 



    //  red, reduced to single-precision float (originally double-precision double): 

    float r1 = (float)pixel_1.x * ((float)x2 - xInput) + (float)pixel_3.x * (xInput - (float)x1);
    float r2 = (float)pixel_2.x * ((float)x2 - xInput) + (float)pixel_4.x * (xInput - (float)x1);

    float dred = r1 * ((float)y2 - yInput) + r2 * (yInput - (float)y1);

    if (dred > 1.0) dred = 1.0; // if (dred > 255.0) dred = 255.0;
    if (dred < 0.0) dred = 0.0;


    //  green, reduced to single-precision float (originally double-precision double): 

    r1 = (float)pixel_1.y * ((float)x2 - xInput) + (float)pixel_3.y * (xInput - (float)x1); // Assumes pixel_1.y is green channel;
    r2 = (float)pixel_2.y * ((float)x2 - xInput) + (float)pixel_4.y * (xInput - (float)x1); // Assumes pixel_1.y is green channel;

    float dgrn = r1 * ((float)y2 - yInput) + r2 * (yInput - (float)y1);

    if (dgrn > 1.0) dgrn = 1.0;
    if (dgrn < 0.0) dgrn = 0.0;


    //  blue, reduced to single-precision float (originally double-precision double): 

    r1 = (float)pixel_1.z * ((float)x2 - xInput) + (float)pixel_3.z * (xInput - (float)x1); // Assumes pixel_1.z is blue channel;
    r2 = (float)pixel_2.z * ((float)x2 - xInput) + (float)pixel_4.z * (xInput - (float)x1); // Assumes pixel_1.z is blue channel;

    float dblu = r1 * ((float)y2 - yInput) + r2 * (yInput - (float)y1);

    if (dblu > 1.0) dblu = 1.0;
    if (dblu < 0.0) dblu = 0.0;


    outPixel = float4(dred, dgrn, dblu, 1.0);  // alpha channel = 1.;


    return retOkay;
}










double2 hyp2in_Transform(double2 somePoint)
{
    double _a00 = hyp2in_ABC_.x; 
    double _a01 = hyp2in_ABC_.y; 
    double _a02 = hyp2in_ABC_.z; 

    double _a10 = hyp2in_DEF_.x; 
    double _a11 = hyp2in_DEF_.y; 
    double _a12 = hyp2in_DEF_.z; 

    double2 otherPoint;
    otherPoint.x = _a00 * somePoint.x + _a01 * somePoint.y + _a02;
    otherPoint.y = _a10 * somePoint.x + _a11 * somePoint.y + _a12;

    return otherPoint;
}




double2 out2hyp_Transform(double2 somePoint)
{
    double _a00 = out2hyp_ABC_.x; 
    double _a01 = out2hyp_ABC_.y; 
    double _a02 = out2hyp_ABC_.z; 

    double _a10 = out2hyp_DEF_.x; 
    double _a11 = out2hyp_DEF_.y; 
    double _a12 = out2hyp_DEF_.z; 

    double2 otherPoint;
    otherPoint.x = _a00 * somePoint.x + _a01 * somePoint.y + _a02;
    otherPoint.y = _a10 * somePoint.x + _a11 * somePoint.y + _a12;

    return otherPoint;
}









bool InsideOrOnCircle(double3 aCircle, double2 somePoint)
{
    //  Any circle can be stored in a double3 by encoding as: 
    //      
    //  double2 _circleCenter = double2(aCircle.x, aCircle.y)
    //    
    //  double _circleRadius = aCircle.z; 
    //    

    double2 _circleCenter = double2(aCircle.x, aCircle.y);   

    double _circleRadius = aCircle.z; 




    double diffX = _circleCenter.x - somePoint.x;

    double diffY = _circleCenter.y - somePoint.y;


    return (diffX * diffX + diffY * diffY <= _circleRadius * _circleRadius); 
}











bool InsideOrOnUnitCircle(double2 somePoint)
{
    //  special case of circle for which _circleCenter = (0, 0)
    //  and _circleRadius = 1.0; 

    double2 _circleCenter = double2(0.00, 0.00); 
    double _circleRadius = 1.00; 
    double3 asACircle = double3(_circleCenter.x, _circleCenter.y, _circleRadius);   

    return InsideOrOnCircle(asACircle, somePoint);  
}










double2 InversionOfPointInACircle(double3 aCircle, double2 somePoint)
{
    //  Any circle can be stored in a double3 by encoding as: 
    //      
    //  double2 _circleCenter = double2(aCircle.x, aCircle.y)
    //    
    //  double _circleRadius = aCircle.z; 
    //    

    double2 _circleCenter = double2(aCircle.x, aCircle.y);   

    double _circleRadius = aCircle.z; 


    double dx = somePoint.x - _circleCenter.x;

    double dy = somePoint.y - _circleCenter.y;

    double rSquared = dx * dx + dy * dy;



    double r2 = _circleRadius * _circleRadius / rSquared;

    double x2 = r2 * dx;

    double y2 = r2 * dy;



    double2 qPoint;

    qPoint.x = _circleCenter.x + x2;

    qPoint.y = _circleCenter.y + y2;


    return qPoint;
}














/*
    
bool FDTContainsPoint(double2 somePoint)
{
    bool retval = false; 

    double FDT_mSlope = FDT.x;
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
*/










bool insideFundamentalRegionFDT(double2 somePoint)
{
    double FDT_mSlope = FDT.x;
    double FDT_geodesicEdgeRadius = FDT.y; 
    double FDT_geodesicEdgeCenterX = FDT.z; 
    double FDT_geodesicEdgeCenterY = FDT.w; 


    if (somePoint.y > FDT_mSlope * somePoint.x) return false;


    if (somePoint.y < 0.0) return false;


    double3 fdtCircle = double3(FDT_geodesicEdgeCenterX, FDT_geodesicEdgeCenterY, FDT_geodesicEdgeRadius); 

    if (InsideOrOnCircle(fdtCircle, somePoint)) return false;


    return true;
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








bool FundamentalRegionReversePixelLookup(double2 aPoint, int maxIterations, out double2 outPoint, inout bool parity)
{
    int d = maxIterations;

    double2 bPt = aPoint;



    while (d >= 0)
    {
        //  
        //  Rotate point to within +/- PI/p    TODO:  Make this more efficient!
        //  

        double theta = atan2(bPt.y, bPt.x);

        double r = sqrt(bPt.x * bPt.x + bPt.y * bPt.y);

        while (theta < 0.0) theta += 2.0 * konst_pi;



        while (theta > konst_pi / (double)SchlafliSymbol.x) 
        {
            theta -= 2.0 * konst_pi / (double)SchlafliSymbol.x;
        }


        double2 cPoint;
        cPoint.x = r * cos(theta);
        cPoint.y = r * sin(theta);


        if (cPoint.y < 0.0) 
        {
            cPoint.y = -cPoint.y;

            //  *parity = (*parity) ? false : true;

            parity = (parity) ? false : true;
        }



        if (insideFundamentalRegionFDT(cPoint)) 
        {
            // out->x = c.x;
            // out->y = c.y;

            outPoint.x = cPoint.x;
            outPoint.y = cPoint.y;

            return false;
        }



        double FDT_geodesicEdgeRadius = FDT.y; 
        double FDT_geodesicEdgeCenterX = FDT.z; 
        double FDT_geodesicEdgeCenterY = FDT.w; 

        double3 fdtCircle = double3(FDT_geodesicEdgeCenterX, FDT_geodesicEdgeCenterY, FDT_geodesicEdgeRadius); 

        bPt = InversionOfPointInACircle(fdtCircle, cPoint);



        //  *parity = (*parity) ? false : true;

        parity = (parity) ? false : true;



        if (insideFundamentalRegionFDT(bPt)) 
        {
            // out->x = b.x;
            // out->y = b.y;

            outPoint.x = bPt.x;
            outPoint.y = bPt.y;

            return false;
        }

        d--;
    }

    return true;
}





//   
//  Interpretation:  
//      pixelRGB.x = redAccum;
//      pixelRGB.y = greenAccum;
//      pixelRGB.z = blueAccum; 
//      


struct gvSharedData
{
    float3          pixelRGB;  //  <========== ghv: cf   BC6HEnclode.hlsl; 
    uint            allOutside;  
    uint            allOutOfBounds; 
    uint            nCounter; 
};


groupshared   gvSharedData   shared_temp[supersamplingRate * supersamplingRate];  // one-dimensional array with card ssRate x ssRate;


[numthreads(supersamplingRate, supersamplingRate, 1)]  //  Each Thread Group contains ssRate x ssRate threads; 
void cs_main(uint3 groupID : SV_GroupID,  uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex) 
{
    shared_temp[GI].pixelRGB.x = 0.0;
    shared_temp[GI].pixelRGB.y = 0.0;
    shared_temp[GI].pixelRGB.z = 0.0;
    shared_temp[GI].allOutside = 1;
    shared_temp[GI].allOutOfBounds = 1; 
    shared_temp[GI].nCounter = 0;


    int maxIterations = 16;    // hard-coded; 


	bool negate = (xu4a.x) ? true : false;



    int xidx = (512 * LoopIndex.x) + groupID.x;
    int yidx = (512 * LoopIndex.y) + groupID.y;
    uint3 gvGroupID = uint3(xidx, yidx, 0); 

    int xx = GTid.x; 
    int yy = GTid.y; 


    double2 a;
    a.x = (double)(xidx + xx * (1.0 / (double)supersamplingRate));
    a.y = (double)(yidx + yy * (1.0 / (double)supersamplingRate));

    a = out2hyp_Transform(a); 

    if (InsideOrOnUnitCircle(a))
    {
        shared_temp[GI].allOutside = 0;

        double2 outputPoint = double2(0.00, 0.00);   

        int index = 0;

        bool parity = false;

        bool err = FundamentalRegionReversePixelLookup(a, maxIterations, outputPoint, parity);

        if (err)
        {
            output_as_uav[gvGroupID.xy] = unreachableColor; // Set pixel color in the outputImage;
        }

        bool negated = false;

        if (negate && parity)
        {
            negated = true;
        }

        double2 b = hyp2in_Transform(outputPoint); // Find corresponding input pixel

        float4 newOutputPixel; 

        int retvalGSP = GetSubPixelFromInputImage(b.x, b.y, newOutputPixel); 

        if (retvalGSP == retOkay)
        {
            shared_temp[GI].allOutOfBounds = 0;

            if (negated) { newOutputPixel = negateRGB(newOutputPixel); }

            shared_temp[GI].pixelRGB.x = newOutputPixel.x; // Assumes that .x is the red channel; 
            shared_temp[GI].pixelRGB.y = newOutputPixel.y; // Assumes that .y is the grn channel; 
            shared_temp[GI].pixelRGB.z = newOutputPixel.z; // Assumes that .z is the blu channel; 


            shared_temp[GI].nCounter = 1; 
        }

    } // if inside or on unit circle;

    //##################################################################

    GroupMemoryBarrierWithGroupSync();

    //##################################################################

    if (GI == 0)
    {
        uint totalOutside = 0;
        uint totalOutOfBounds = 0; 
        uint nTotalCount = 0; 
        float3 totalRGB = float3(0.0, 0.0, 0.0); 

        for (int jj = 0; jj < supersamplingRate * supersamplingRate; jj++)
        {
            totalOutside += shared_temp[jj].allOutside; 
            totalOutOfBounds += shared_temp[jj].allOutOfBounds; 
            nTotalCount += shared_temp[jj].nCounter; 
            totalRGB += shared_temp[jj].pixelRGB; 
        }

        if (totalOutside > 0)
        {
            output_as_uav[gvGroupID.xy] = outOfBoundsColor; // Set pixel color at (xidx, yidx) in the outputImage;
        }
        else if (totalOutOfBounds)
        {
            output_as_uav[gvGroupID.xy] = nonSourceColor; // Set pixel at (xidx, yidx) in the outputImage;
        }
        else
        {

            float4 renderedOutputPixel = float4(
                (double)totalRGB.x / (double)nTotalCount + mystery,
                (double)totalRGB.y / (double)nTotalCount + mystery,
                (double)totalRGB.z / (double)nTotalCount + mystery,
                1.0
                );

            output_as_uav[gvGroupID.xy] = renderedOutputPixel; // Set pixel at (xidx, yidx)  in the outputImage;
        }
    }

    //  TODO:   gvAsync_LaunchFileSavePicker(pt2, fmhOutput.datalen);
    
    //  TODO:   m_main->m_sceneRenderer->e_TextureSRV0.Reset();
}




