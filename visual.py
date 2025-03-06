import wave
import matplotlib.pyplot as plt
import numpy as np

wav_obj = wave.open('/Users/apple/Desktop/Spring25/granBasics/env4.wav', 'rb')

#getting wave file data 
sample_freq = wav_obj.getframerate()
n_samples = wav_obj.getnframes()
t_audio = n_samples/sample_freq
n_channels = wav_obj.getnchannels()
signal_wave = wav_obj.readframes(n_samples)

#creating an array of the signal
signal_array = np.frombuffer(signal_wave, dtype=np.int16)

l_channel = signal_array[0::2]
r_channel = signal_array[1::2]

times = np.linspace(0, n_samples/sample_freq, num=n_samples)

grainduration = 0.1
grainsamples = int(grainduration * sample_freq)

# Slice out the first grain_samples from the left channel
grain_l = l_channel[:grainsamples]

# Create time array for just the grain
grain_times = np.linspace(0, grainduration, num=grainsamples)


#plt.figure(figsize=(15, 5))
#plt.plot(times, l_channel)
#plt.title('Left Channel')
#plt.ylabel('Signal Value')
#plt.xlabel('Time (s)')
#plt.xlim(0, t_audio)
#plt.show()

plt.figure(figsize=(10, 4))
plt.plot(grain_times, grain_l)
plt.title('100 ms Grain (Left Channel)')
plt.xlabel('Time (s)')
plt.ylabel('Amplitude')
plt.show()