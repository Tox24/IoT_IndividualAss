/*
  Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define SECRET
#define THINGNAME "heltec32"

#define WIFI_SSID "MartinRouterKing"
#define WIFI_PASSWORD "5qktpczupay7sx9"
#define AWS_IOT_ENDPOINT "mqtts://av7nzftewgtl0-ats.iot.eu-north-1.amazonaws.com"

// Amazon Root CA 1
static const char* AWS_CERT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";


// Device Certificate
static const char* AWS_CERT_CRT = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIUQ7AAamHmK4NP1UPG7cAJILyzfEowDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDQyMDE2MTQx\n" \
"OFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK8mkBB0khQ+3dmrpUs7\n" \
"rzoTE6UyX1U9BI/UKc7oIVbS/1cmzvhQqQfghtnd2UBtbbbTrw4ZOpMRZ9pzXccu\n" \
"pUKnUYTr8wv24WprrNImT5TQ2Eot37wlcT0Nds/+1eiaMGxRhav8oslHM0uEZut1\n" \
"sutUPwzNQdHSatvG+hZkOtIrL2OMQQZ+adVsOi4Fb6B9yG3JGvU0hiaNqj3XBxd2\n" \
"6NxKUkgbMa1MTHujLCqmOYv/JCNo/YA56a5IV3FtRvgOYEqtwZQpRPd0ED8xjtub\n" \
"d6VM6bq4hPZoowbsLmWvUU8ak3nmAvgGykm1l4/IcLHM9bB7uGEcozXKFLdWQrSt\n" \
"7iMCAwEAAaNgMF4wHwYDVR0jBBgwFoAUTMHt6W5HSALoIIT3sLxcOwOHauowHQYD\n" \
"VR0OBBYEFMvgcpdh7LXxybce6tOqKuOv+HfUMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBwIuFQbJEAr438nLytVg9ZgX7r\n" \
"+1bO0Or4ZbP19ImeRyD5adaVPSwpaj4OTlFQ4AWt2eXGIB/SZioGnNEMr3PB3Lrt\n" \
"7MKK4G1SOmhRio/iZ2Qq+v4TXi5U5M/KBWC5fTMvbdvwtm5WcimsKg+bnkUkJhSx\n" \
"VvyoRy88dL/Xqu1/tx1r8laW1W05uAe1tOumCeIi4vS78zvdhVVR3Xc+VN5e4nJM\n" \
"BZSJ1cvlUJ368iHpOoZd/nv4lPawU3TaKZFvWs3UvG4WhRlJghiHaPBtYNF22ipD\n" \
"F+UV8b1oBUoeualxNNhEI00ULxxZgQUp9yZQtCAdKgtqXUoG8eBNGguP+BIe\n" \
"-----END CERTIFICATE-----\n";

// Device Private Key
static const char* AWS_CERT_PRIVATE = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEogIBAAKCAQEAryaQEHSSFD7d2aulSzuvOhMTpTJfVT0Ej9QpzughVtL/VybO\n" \
"+FCpB+CG2d3ZQG1tttOvDhk6kxFn2nNdxy6lQqdRhOvzC/bhamus0iZPlNDYSi3f\n" \
"vCVxPQ12z/7V6JowbFGFq/yiyUczS4Rm63Wy61Q/DM1B0dJq28b6FmQ60isvY4xB\n" \
"Bn5p1Ww6LgVvoH3Ibcka9TSGJo2qPdcHF3bo3EpSSBsxrUxMe6MsKqY5i/8kI2j9\n" \
"gDnprkhXcW1G+A5gSq3BlClE93QQPzGO25t3pUzpuriE9mijBuwuZa9RTxqTeeYC\n" \
"+AbKSbWXj8hwscz1sHu4YRyjNcoUt1ZCtK3uIwIDAQABAoIBADIjvvAG/t5u1MGA\n" \
"QpRT5KiiV2heEC5thkXKPaGukASz6EbBpFQvewP3QYNS3+NysAq7dIx7qmn5fJpr\n" \
"ljfz4XlAPrTneq89IHB/nHyYXQXD93bcxQSuT0lj9lQ6pm+s3BnWCIcgjlVCXavL\n" \
"nk7fZbW6a0Y16BaxvsdloagRT0lkfBYifdY7NTKO2yS1q6X5qfS3EoyXQRaHfcmw\n" \
"gcYEO2iBwNLibwiCMJNOVgWNvOA0yT4AwSXimjYFSSWOfMCxMKWBL/ijx2vfwbNA\n" \
"1kQCkG+z4Ej70wJh8S085oF03xnvr8daUV/fYEKvKwjjGt8IemIFyecaP6tgZzcr\n" \
"CmFOdJECgYEA3RRJEuGbuMYNmDGVuQY6agdNw8+d0f22+GpXvnt07HkQuorcEQlv\n" \
"8B2x+b+z7AJsT8IlBV5+VNB6fM8GJXpV02DRDq0RR/CgazpYsrdrmcKBfBQgcgTZ\n" \
"X69cNuvSKojFoviyEC3cR57gDklx46i+jv9olp8Uj3eizLKcnP3utCsCgYEAytET\n" \
"gP0YWlViE1EDPd+huiV2H9HSnHJoBtWGvcZSZFrbsdkC5ezOQBnI89CDD3AyprY/\n" \
"p8CG3kiO8mEg6+BLp/Z1gaG8560srCtAWkfmLQZyFDGjcM6WWdq4X7+ujwcU7uFf\n" \
"tnKNhOQEDpa/g5rFGz1mpG1VOx1bHKHVgOvbWekCgYAD0ZHhjZwO+PzLIDAh3gWs\n" \
"RywEsdVcBzHd4JhbZe6DFyQ+1J8wfCU+1IR6d+E3tmMAja3uBc/QzzkOZtUIWLCa\n" \
"0hVKV5rwzys2Lu/RRnHJWh66ce6NiZ/nkzPYjwI2Ud54DiulM+WWJxxfos0gzY1d\n" \
"EvRPhbfpMnvRZVRnMcupuQKBgDxT8H8/yYylNSUVxecTrCGYnwhb+0+54COYRBoS\n" \
"8dMUC6FdMbV+uOLsSI/th+6Bqy9XqpuVcwiPgKWsKoS/FQIDF3TVzUEEi/MyclPo\n" \
"axkdf7VuBnW5nZNgsdMZSy0UKC/eLgAxFtNel65XPORClzrBCtUCCIYq9z0PDljo\n" \
"zzhxAoGAPvCY4H7azI4LWnutFbxmEvj4JnbfubpPpHJwLzyDPWCW1xMvJK2EIFog\n" \
"bYVlbJh4clP0m25RRCoFMTZ5e4QWyCjkm0z8sMAE3CbuOJB7kgGsjSTErrKWga7x\n" \
"D0LtdJmZSmzyalRshA7KokivmPrm36u4AGyXYduzFIKuE4H/t7o=\n" \
"-----END RSA PRIVATE KEY-----\n";
