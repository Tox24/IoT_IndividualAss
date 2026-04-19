import numpy as np
import sounddevice as sd

sps = 44100
freq_hz = 5
duration_s = 60.0
attenuation = 0.8

each_sample_number = np.arange(duration_s * sps)

waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps)
waveform_quiet = waveform * attenuation

while True:
    sd.play(waveform_quiet, sps)
    sd.wait()