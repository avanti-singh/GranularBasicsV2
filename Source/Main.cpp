#include <iostream>
#include <vector>
#include <algorithm>
#include <JuceHeader.h>

// Convert time (in seconds) to samples
float timetosamples(float time, int samplerate) {
    return time * samplerate;
}

// Circular buffer (delay line) class
class Circularbuff {
public:
    Circularbuff(int size) : buffer(size, 0.0f), writepos(0), buffersize(size) {}

    float process(float input, int delaysamples) {
        buffer[writepos] = input;
        int readpos = (writepos - delaysamples + buffersize) % buffersize;
        float buffoutput = buffer[readpos];
        writepos = (writepos + 1) % buffersize;
        return buffoutput;
    }

private:
    std::vector<float> buffer;
    int writepos;
    int buffersize;
};

// Envelope function to shape each grain
void env(juce::AudioBuffer<float>& buffer, float attacktime, float decaytime, float sustainlevel, float releasetime, float samplerate) {

    if (attacktime <= 0.0f) attacktime = 0.1f;
    if (decaytime <= 0.0f) decaytime = 0.1f;
    if (releasetime <= 0.0f) releasetime = 0.1f;

    float totalattacksamples = timetosamples(attacktime, samplerate);
    float totaldecaysamples  = timetosamples(decaytime, samplerate);
    float totalreleasesamples = timetosamples(releasetime, samplerate);

    int totalsamples = buffer.getNumSamples();
    for (int chan = 0; chan < buffer.getNumChannels(); chan++) {
        float* channelData = buffer.getWritePointer(chan);
        float attackEnd = totalattacksamples;
        float decayEnd  = totalattacksamples + totaldecaysamples;
        float sustainEnd = totalsamples - totalreleasesamples;
        if (sustainEnd < decayEnd)
            sustainEnd = decayEnd;
        
        for (int i = 0; i < totalsamples; i++) {
            float amplitude = 0.0f;
            if (i < attackEnd) {
                amplitude = static_cast<float>(i) / attackEnd;
            }
            else if (i < decayEnd) {
                amplitude = 1.0f + (static_cast<float>(i) - attackEnd) * ((sustainlevel - 1.0f) / (decayEnd - attackEnd));
            }
            else if (i < sustainEnd) {
                amplitude = sustainlevel;
            }
            else {
                if (totalsamples - 1 > sustainEnd)
                    amplitude = sustainlevel + (static_cast<float>(i) - sustainEnd) * (-sustainlevel) / (totalsamples - 1 - sustainEnd);
                else
                    amplitude = 0.0f;
            }
            channelData[i] *= amplitude;
        }
    }
}

// Grain class: extracts a segment from the input buffer and applies an envelope.
class Grain {
public:
    juce::AudioBuffer<float> buffer;

    Grain(juce::AudioBuffer<float>& inputBuffer, int startSample, int numSamples, float samplerate) {
        buffer.setSize(inputBuffer.getNumChannels(), numSamples);
        for (int chan = 0; chan < inputBuffer.getNumChannels(); chan++) {
            const float* src = inputBuffer.getReadPointer(chan);
            float* dest = buffer.getWritePointer(chan);
            std::copy(src + startSample, src + startSample + numSamples, dest);
        }

        env(buffer, 0.01f, 0.01f, 0.8f, 0.01f, samplerate);
    }
};

int main() {
    // Input and output file paths
    std::string inputwav = "/Users/apple/Desktop/Spring25/granBasics/input.wav";
    std::string outputwav = "/Users/apple/Desktop/Spring25/granBasics/grain1.wav";

    juce::File inputfile(inputwav);
    juce::File outputfile(outputwav);

    // Set up JUCE audio formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // Open the input file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputfile));
    if (!reader) {
        std::cout << "Failed to open input file." << std::endl;
        return 1;
    }

    int samplerate   = static_cast<int>(reader->sampleRate);
    int totalsamples = static_cast<int>(reader->lengthInSamples);
    int numchannels  = reader->numChannels;

    // Read the entire file into a buffer
    juce::AudioBuffer<float> inputBuffer;
    inputBuffer.setSize(numchannels, totalsamples);
    reader->read(&inputBuffer, 0, totalsamples, 0, true, true);

    // Set grain and scheduling parameters
    float grainDurationSec     = 0.1f; 
    float timeBetweenGrainsSec = 0.1f; 
    int grainSamples     = static_cast<int>(timetosamples(grainDurationSec, samplerate));
    int interonsetSamples = static_cast<int>(timetosamples(timeBetweenGrainsSec, samplerate));

    // Determine how many grains can be extracted from the input buffer
    int numGrains = (totalsamples - grainSamples) / interonsetSamples + 1;
    std::vector<Grain> grains;
    grains.reserve(numGrains);

    // Extract grains from the input buffer
    for (int i = 0; i < numGrains; i++) {
        int startSample = i * interonsetSamples;
        if (startSample + grainSamples > totalsamples)
            break;
        grains.emplace_back(inputBuffer, startSample, grainSamples, static_cast<float>(samplerate));
    }

    // Compute the length of the final output (to accommodate scheduled grains)
    int finalOutputLength = (numGrains - 1) * interonsetSamples + grainSamples;
    juce::AudioBuffer<float> outputBuffer;
    outputBuffer.setSize(numchannels, finalOutputLength);
    outputBuffer.clear();

    // Create one delay (circular buffer) per channel.
    std::vector<Circularbuff> delayLines;
    int delayBufferSize = finalOutputLength; 
    for (int chan = 0; chan < numchannels; chan++) {
        delayLines.emplace_back(delayBufferSize);
    }
    int delaySamples = interonsetSamples; 

    // Process each grain:
    // For each grain, process its samples through the delay line and schedule it in the final output.
    for (int i = 0; i < static_cast<int>(grains.size()); i++) {
        int grainStart = i * interonsetSamples;
        Grain& g = grains[i];
        int grainNumSamples = g.buffer.getNumSamples();
        for (int chan = 0; chan < numchannels; chan++) {
            const float* grainData = g.buffer.getReadPointer(chan);
            float* outData = outputBuffer.getWritePointer(chan);
            for (int j = 0; j < grainNumSamples; j++) {
                int pos = grainStart + j;
                if (pos < finalOutputLength) {
                    float delayedSample = delayLines[chan].process(grainData[j], delaySamples);
                    outData[pos] += delayedSample;
                }
            }
        }
    }

    // Write the final output buffer to a WAV file
    std::unique_ptr<juce::FileOutputStream> fileStream(outputfile.createOutputStream());
    if (!fileStream) {
        std::cout << "Failed to create output stream." << std::endl;
        return 1;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        formatManager.findFormatForFileExtension("wav")->createWriterFor(
            fileStream.get(),
            reader->sampleRate,
            static_cast<unsigned int>(numchannels),
            16,
            {},
            0));

    if (!writer) {
        std::cout << "Failed to create writer." << std::endl;
        return 1;
    }

    writer->writeFromAudioSampleBuffer(outputBuffer, 0, outputBuffer.getNumSamples());

    std::cout << "Granular synthesis processing complete." << std::endl;
    return 0;
}
