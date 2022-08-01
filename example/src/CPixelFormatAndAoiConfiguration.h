#pragma once

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>

class CPixelFormatAndAoiConfiguration : public Pylon::CConfigurationEventHandler
{
public:
    void OnOpened( Pylon::CInstantCamera& camera )
    {
        try
        {
            // Allow all the names in the namespace GenApi to be used without qualification.
            using namespace Pylon;

            // Get the camera control object.
            GenApi::INodeMap& nodemap = camera.GetNodeMap();

            // Get the parameters for setting the image area of interest (Image AOI).
            CBooleanParameter scaling(nodemap, "BslScalingEnable");
            scaling.TrySetValue(true);

            CIntegerParameter width( nodemap, "Width" );
            CIntegerParameter height( nodemap, "Height" );
            width.TrySetValue(1024);
            height.TrySetValue(768);
 
            std::cout << "Info: " << "OnOpen " << std::endl;
        }
        catch (const Pylon::GenericException& e)
        {
            throw RUNTIME_EXCEPTION( "Could not apply configuration. const GenericException caught in OnOpened method msg=%hs", e.what() );
        }
    }
};