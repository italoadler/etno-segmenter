#include "../marsystems/buffer.hpp"
#include "../marsystems/vampsink.hpp"
#include <marsyas/MarSystemManager.h>
#include <iostream>

using namespace Marsyas;

int main()
{
    MarSystemManager mng;

    MarSystem* protoBuffer = new Marsyas::Buffer("protoBuffer");
    mng.registerPrototype("Buffer", protoBuffer);

    MarSystem* protoVampSink = new Marsyas::VampSink("protoVampSink");
    mng.registerPrototype("VampSink", protoVampSink);

    MarSystem *buffer = mng.create("Buffer", "buffer");
    VampSink *vampSink = static_cast<VampSink*>( mng.create("VampSink", "vampSink") );
    //MarSystem *log = mng.create("PlotSink", "log");
    buffer->addMarSystem( vampSink );

    int inputObservations = 2;
    int inputSamples = 6;

    //log->setControl("mrs_bool/sequence", false);
    //log->setControl("mrs_bool/messages", true);

    Vamp::Plugin::FeatureSet featureSet;

    vampSink->setFeatureSet( &featureSet );

    buffer->setControl("mrs_natural/inObservations", inputObservations);
    buffer->setControl("mrs_natural/inSamples", inputSamples);
    buffer->setControl("mrs_natural/blockSize", 10);
    buffer->setControl("mrs_natural/hopSize", 7);

    buffer->update();

    mrs_natural outObservations = buffer->getControl("mrs_natural/onObservations")->to<mrs_natural>();
    mrs_natural outSamples = buffer->getControl("mrs_natural/onSamples")->to<mrs_natural>();

    realvec input(inputObservations, inputSamples);
    realvec output(outObservations, outSamples);

    int x = 0;
    for(int i = 0; i < 20; ++i)
    {
        featureSet.clear();

        for (int s = 0; s < inputSamples; ++s) {
            for (int o = 0; o < inputObservations; ++o) {
                input(o, s) = x + o * 100;
            }
            ++x;
        }

        std::cout << "*** proces..." << std::endl;

        buffer->process(input, output);

        std::vector<Vamp::Plugin::Feature> & featureVector = featureSet[0];

        std::cout << "*** features:" << std::endl;

        for(int f = 0; f < featureVector.size(); ++f)
        {
            const Vamp::Plugin::Feature & feature = featureVector[f];
            for(int o = 0; o < feature.values.size(); ++o)
                std::cout << feature.values[o] << " ";
            std::cout << std::endl;
        }
    }

    return 0;
}
