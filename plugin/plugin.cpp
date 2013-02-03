#include "plugin.hpp"
#include "../modules/pipeline.hpp"

#include <vamp/vamp.h>

#include <sstream>
#include <iostream>

#define SEGMENTER_LOGGING 1
#define SEGMENTER_LOGGING_SEPARATOR '\t'

using namespace std;

namespace Segmenter {

Plugin::Plugin(float inputSampleRate):
    Vamp::Plugin(inputSampleRate),
    m_blockSize(0),
    m_pipeline(0),
    m_logFrame(0),
    m_logFrameLimit(0)
{}

Plugin::~Plugin()
{
    delete m_pipeline;
}

string Plugin::getIdentifier() const
{
    return "audiosegmenter";
}

string Plugin::getName() const
{
    return "Audio Segmenter";
}

string Plugin::getDescription() const
{
    return "Segment audio into segments of various sound types";
}


string Plugin::getMaker() const
{
    return "Matija Marolt & Jakob Leben";
}

string Plugin::getCopyright() const
{
    return ("Copyright 2013 Matija Marolt & Jakob Leben, "
            "Faculty of Computer and Information Science, University of Ljubljana.");
}

int Plugin::getPluginVersion() const
{
    return 1;
}

Vamp::Plugin::InputDomain Plugin::getInputDomain() const
{
    return TimeDomain;
}

size_t Plugin::getMinChannelCount() const
{
    return 1;
}

size_t Plugin::getMaxChannelCount() const
{
    return 1;
}

size_t Plugin::getPreferredBlockSize() const
{
    // FIXME: do not assume a specific downsampling ratio
    return 4096; // == 512 * 4 (downsampling ratio) * 2 (resampler buffering)
}

size_t Plugin::getPreferredStepSize() const
{
    return getPreferredBlockSize();
}

bool Plugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    std::cout << "**** Plugin::initialise:"
        << " " << channels
        << " " << stepSize
        << " " << blockSize
        << " " << std::endl;

    if (channels != 1)
        return false;

    if (blockSize != stepSize)
        return false;

    m_blockSize = blockSize;

    createPipeline();

#if 0
    int featureFrames = std::ceil(
        m_pipeline->fourierContext().stepSize
        * ((double) m_inputSampleRate / m_pipeline->fourierContext().sampleRate) );

    m_featureDuration = Vamp::RealTime::frame2RealTime( featureFrames, m_inputSampleRate );
#endif

#if SEGMENTER_LOGGING
    if (m_file.is_open())
        m_file.close();

    m_file.open("/home/jakob/programming/audiosegmenter/log.txt", std::ios_base::out);
    if (m_file.fail())
        std::cout << "*** Could not open logging file" << std::endl;
    else
        std::cout << "*** Successfully opened logging file" << std::endl;

    m_file << std::fixed;

    m_logFrame = 0;
    m_logFrameLimit = 30 * m_pipeline->fourierContext().sampleRate;
#endif
}

void Plugin::createPipeline()
{
    delete m_pipeline;

    InputContext inCtx;
    inCtx.sampleRate = m_inputSampleRate;
    inCtx.blockSize = m_blockSize;

    FourierContext fCtx;
#if SEGMENTER_NO_RESAMPLING
    fCtx.sampleRate = m_inputSampleRate;
    fCtx.blockSize = 2048;
    fCtx.stepSize = 1024;
#else
    fCtx.sampleRate = 11025;
    fCtx.blockSize = 512;
    fCtx.stepSize = 256;
#endif

    StatisticContext statCtx;
    statCtx.blockSize = 132;
    statCtx.stepSize = 22;

    std::cout << "*** Segmenter: blocksize=" << fCtx.blockSize << " stepSize=" << fCtx.stepSize << std::endl;

    m_pipeline = new Pipeline( inCtx, fCtx, statCtx );
}

void Plugin::reset()
{
    std::cout << "**** Plugin::reset" << std::endl;
    createPipeline();
}

Vamp::Plugin::OutputList Plugin::getOutputDescriptors() const
{
    OutputList list;

#if 0
    int featureFrames = std::ceil(
        m_pipeline->fourierContext().stepSize
        * ((double) m_inputSampleRate / m_pipeline->fourierContext().sampleRate) );

    double featureSampleRate = (double) m_inputContext.sampleRate / featureFrames;

    OutputDescriptor outStat;
    outStat.identifier = "features";
    outStat.name = "Features";
    outStat.sampleType = OutputDescriptor::FixedSampleRate;
    outStat.sampleRate = featureSampleRate;
    outStat.hasFixedBinCount = true;
    outStat.binCount = 1;

    list.push_back(outStat);
#endif

    OutputDescriptor outClasses;
    outClasses.identifier = "classification";
    outClasses.name = "Classification";
    outClasses.sampleType = OutputDescriptor::FixedSampleRate;
    if (m_pipeline) {
        outClasses.sampleRate =
            (double) m_pipeline->fourierContext().sampleRate /
            (m_pipeline->statisticContext().stepSize * m_pipeline->fourierContext().stepSize);
    } else {
        outClasses.sampleRate = 1;
    }
    outClasses.hasFixedBinCount = true;
    outClasses.binCount = 1;

    /*outClasses.binCount = classifier ? classifier->classNames().count() : 0;
    if (classifier) outClasses.binNames = classifier->classNames();*/
    /*
    outClasses.isQuantized;
    outClasses.quantizeStep = 1;
    outClasses.hasKnownExtents = true;
    outClasses.minValue = 1;
    outClasses.maxValue = 5;
    */

    list.push_back(outClasses);

    return list;
}

Vamp::Plugin::FeatureSet Plugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    return getFeatures( inputBuffers[0], timestamp );
}

Vamp::Plugin::FeatureSet Plugin::getRemainingFeatures()
{
    return getFeatures( 0, Vamp::RealTime() );
}

Vamp::Plugin::FeatureSet Plugin::getFeatures(const float * input, Vamp::RealTime timestamp)
{
    FeatureSet features;

    m_pipeline->computeStatistics( input );
    m_pipeline->computeClassification( features[0] );

#if 0
    for ( blockFrame = 0;
          blockFrame <= (int) m_resampBuffer.size() - m_procContext.blockSize;
          blockFrame += m_procContext.stepSize )
    {
        Feature basicFeatures;
        basicFeatures.hasTimestamp = true;
        basicFeatures.timestamp = m_featureTime;
#if SEGMENTER_NEW_FEATURES
        //basicFeatures.values.push_back( fourHzMod->output() );
        basicFeatures.values.push_back( cepstralFeatures->tonality() );
#endif
        /*
        basicFeatures.values.push_back( chromaticEntropy->output() );
        basicFeatures.values.push_back( mfcc->output()[2] );
        basicFeatures.values.push_back( mfcc->output()[3] );
        basicFeatures.values.push_back( mfcc->output()[4] );
        basicFeatures.values.push_back( energy->output() );
        */

        features[0].push_back(basicFeatures);

        m_featureTime = m_featureTime + m_featureDuration;
    }

    for (int i = 0; i < m_statsBuffer.size(); ++i)
    {}
#endif

    return features;
}

} // namespace Segmenter
