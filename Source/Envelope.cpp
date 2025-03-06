#include <JuceHeader.h>
#include <iostream>
#include <vector>


float timetosamples(float time, int samplerate) {
   return time * samplerate;
}


void env(juce::AudioBuffer<float>& buffer, float attacktime, float decaytime, float sustainlevel, float releasetime, float samplerate) {


if (attacktime <= 0.0f)
   attacktime = 0.1f;


if (decaytime <= 0.0f)
   decaytime = 0.1f;


if (releasetime <= 0.0f)
   releasetime = 0.1f;


float totalattacksamples = timetosamples(attacktime, samplerate);
float totaldecaysamples = timetosamples(decaytime, samplerate);
float totalreleasesamples = timetosamples(releasetime, samplerate);


int totalsamples = buffer.getNumSamples();


for (int chan = 0; chan < buffer.getNumChannels(); chan++)
{
float* channelData = buffer.getWritePointer(chan);


float attackEnd = totalattacksamples;
float decayEnd  = totalattacksamples + totaldecaysamples;
float sustainEnd = totalsamples - totalreleasesamples;


if (sustainEnd < decayEnd)
       sustainEnd = decayEnd;


for (int i = 0; i < totalsamples; i++) {
   float amplitude = 0.0f;


   if (i < attackEnd) {
       amplitude = (float)i / attackEnd;
   }
   else if (i < decayEnd) {
       amplitude = 1.0f + ((float)i - attackEnd) * ((sustainlevel - 1.0f) / (decayEnd - attackEnd));
   }
   else if (i < sustainEnd) {
       amplitude = sustainlevel;
   }
   else {
       if (totalsamples - 1 > sustainEnd)
           amplitude = sustainlevel + ((float)i - sustainEnd) * (- sustainlevel) /(totalsamples - 1 - sustainEnd);
       else
           amplitude = 0.0f;
   }
  
   channelData[i] *= amplitude;
}
}
}


int main() {


// Input and output file paths
std::string inputwav = "/Users/apple/Desktop/Spring25/granBasics/input.wav";
std::string outputwav = "/Users/apple/Desktop/Spring25/granBasics/env4.wav";


juce::File inputfile(inputwav);
juce::File outputfile(outputwav);


// Registering basic audio formats
juce::AudioFormatManager formatManager;
formatManager.registerBasicFormats();


// Creating a unique pointer for the reader
std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputfile));


if (!reader) {
   std::cout << "Failed to open input file." << std::endl;
   return 1;
}


juce::AudioBuffer<float> buffer;
buffer.setSize(reader->numChannels, reader->lengthInSamples);
reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);


int samplerate = static_cast<int>(reader->sampleRate);
int totalsamples = buffer.getNumSamples();


float fileduration = (float) totalsamples / (float) samplerate;


float attackFrac  = 0.5f;
float decayFrac   = 0.1f;
float sustainFrac = 0.3f;
float releaseFrac = 0.2f;


float attackTime  = fileduration * attackFrac; 
float decayTime   = fileduration * decayFrac;
float releaseTime = fileduration * releaseFrac;
float sustainLevel = 0.5f;


//APPLY ENVELOPE FUNCTION
env(buffer, attackTime, decayTime, sustainLevel, releaseTime, (float)samplerate);


// Creating a unique pointer for the writer
std::unique_ptr<juce::AudioFormatWriter> writer(
   formatManager.findFormatForFileExtension("wav")->createWriterFor(
       new juce::FileOutputStream(outputfile),
       reader->sampleRate,
       reader->numChannels,
       16,
       {},
       0));


if (!writer) {
   std::cout << "Failed to create writer." << std::endl;
   return 1;
}


writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());


std::cout << "Envelope processing complete." << std::endl;
return 0;


}


//for loop over the buffer, if sample falls under a certain stage, input it into the function of that stage
//if it's in the attack stage, linear interpolate from 0 to max value (1?)
//if it's in the decay stage - linear interpolate from max value to sustain value
//if it's in the sustain stage - keep it at the sustain value
//if it's in the release stage - linear interpolate from sustain value to 0
/*y0 + (x - x0) * ((y1 - y0) / (x1 - x0))


- `y0` is the starting value of the envelope.
- `y1` is the ending value of the envelope.
- `x` is the current index of the buffer that we are applying to the envelope.
- `x0` is the starting index of the buffer.
- `x1` is the ending index of the buffer.
*/


//convert times to samples, iterate over buffer to check which stage each sample is in
//lerp?
