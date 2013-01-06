#ifndef SEGMENTER_RESAMPLER_HPP_INCLUDED
#define SEGMENTER_RESAMPLER_HPP_INCLUDED

//#include "module.hpp"

#include <samplerate.h>
#include <vector>
#include <iostream>
#include <cstdlib>

namespace Segmenter {

using std::vector;

class Resampler
{
    const int m_inSampleRate;
    const int m_outSampleRate;

    vector<float> m_inBuffer;
    SRC_STATE *m_srcState;
    SRC_DATA m_srcData;

public:
    Resampler( int inputSampleRate, int outputSampleRate = 11025 ):
        m_inSampleRate(inputSampleRate),
        m_outSampleRate(outputSampleRate)
    {
        int error = 0;
        const int channelCount = 1;
        m_srcState = src_new( SRC_LINEAR, channelCount, &error );
        if (error) {
            std::cout << "Resampler ERROR: " << src_strerror(error) << std::endl;
            return;
        }

        double conversionRatio = (double) m_outSampleRate / m_inSampleRate;

        m_srcData.src_ratio = conversionRatio;
    }

    ~Resampler()
    {
        src_delete(m_srcState);
    }

    void process( const float *data, int size, std::vector<float> & outBuffer )
    {
        m_inBuffer.insert( m_inBuffer.end(), data, data + size );

        int outputIndex = outBuffer.size();
        int outputSize = m_inBuffer.size() * m_srcData.src_ratio;
        outBuffer.resize( outBuffer.size() + outputSize );

        m_srcData.data_in = m_inBuffer.data();
        m_srcData.input_frames = m_inBuffer.size();
        m_srcData.data_out = outBuffer.data() + outputIndex;
        m_srcData.output_frames = outputSize;
        m_srcData.end_of_input = false;

        int error = src_process( m_srcState, &m_srcData );

        if (error) {
            std::cout << "Resampler ERROR: " << src_strerror(error) << std::endl;
            m_inBuffer.clear();
            return;
        }

        m_inBuffer.erase( m_inBuffer.begin(), m_inBuffer.begin() + m_srcData.input_frames_used );
        outBuffer.resize( outputIndex + m_srcData.output_frames_gen );
    }
/*
    void processRemainingData( std::vector<float> & output )
    {
        m_srcData.data_in = m_inBuffer.data();
        m_srcData.input_frames = m_inBuffer.size();
        m_srcData.end_of_input = true;

        int error = src_process( m_srcState, &m_srcData );

        m_inBuffer.clear();

        if (error)
            std::cout << "Resampler ERROR: " << src_strerror(error) << std::endl;
    }
*/

};

} // namespace Segmenter

#endif // SEGMENTER_RESAMPLER_HPP_INCLUDED
