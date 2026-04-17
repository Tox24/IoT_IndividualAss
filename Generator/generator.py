import numpy as np
import sounddevice as sd
import time

sps = 44100
freq_hz = 10000
duration_s = 5.0
attenuation = 0.3

each_sample_number = np.arange(duration_s * sps)

waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps)
waveform_quiet = waveform * attenuation

sd.play(waveform_quiet, sps)
time.sleep(duration_s)
sd.stop()