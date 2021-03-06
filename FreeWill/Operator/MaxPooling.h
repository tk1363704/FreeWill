#ifndef MAXPOOLING_H
#define MAXPOOLING_H

#include "Operator.h"
#include <cudnn.h>

namespace FreeWill
{
    template<DeviceType DeviceUsed = DeviceType::CPU_NAIVE, typename DataType = float>
    class MaxPooling : public Operator<DeviceUsed>
    {
    protected:
        using Operator<DeviceUsed>::input;
        using Operator<DeviceUsed>::output;
        using Operator<DeviceUsed>::m_deviceId;

        cudnnPoolingDescriptor_t m_poolingDescriptor;
        cudnnTensorDescriptor_t m_inputTensorDescriptor;
        cudnnTensorDescriptor_t m_outputTensorDescriptor;


    public:
        MaxPooling(unsigned int deviceId = 0)
            :Operator<DeviceUsed>({"Input"},{"Output", "SwitchX", "SwitchY"}, deviceId),
            m_poolingDescriptor(0),
            m_inputTensorDescriptor(0),
            m_outputTensorDescriptor(0)
        {
            CHECK_GPU;

            if constexpr (DeviceUsed == DeviceType::GPU_CUDA)
            {
                RUN_CUDNN(cudnnCreatePoolingDescriptor( &m_poolingDescriptor ));
                RUN_CUDNN(cudnnCreateTensorDescriptor (&m_inputTensorDescriptor));
                RUN_CUDNN(cudnnCreateTensorDescriptor(&m_outputTensorDescriptor));
            }
        }

        ~MaxPooling()
        {
            CHECK_GPU;

            if constexpr (DeviceUsed == DeviceType::GPU_CUDA)
            {
                RUN_CUDNN(cudnnDestroyPoolingDescriptor(m_poolingDescriptor));
                RUN_CUDNN(cudnnCreateTensorDescriptor(&m_inputTensorDescriptor));
                RUN_CUDNN(cudnnCreateTensorDescriptor(&m_outputTensorDescriptor));

                m_poolingDescriptor = 0;
                m_inputTensorDescriptor = 0;
                m_outputTensorDescriptor = 0;
            }
        }

        virtual bool init() override
        {
            CHECK_GPU;

            FAIL_IF (!input("Input") || !output("Output"));

            FAIL_IF (input("Input")->shape().dimension() != 4);
            
            FAIL_IF (output("Output")->shape().dimension() != 4);

            if constexpr (DeviceUsed == DeviceType::CPU_NAIVE)
            {
                FAIL_IF(!output("SwitchX") || !output("SwitchY"));

                FAIL_IF (output("SwitchX")->shape() != output("Output")->shape());

                FAIL_IF (output("SwitchY")->shape() != output("Output")->shape());
            }

            FAIL_IF (input("Input")->shape()[0] != output("Output")->shape()[0]);

            FAIL_IF (input("Input")->shape()[1] != 2 * output("Output")->shape()[1]);

            FAIL_IF (input("Input")->shape()[2]!=2*output("Output")->shape()[2]);

            FAIL_IF (input("Input")->shape()[3]!=output("Output")->shape()[3]);

            if constexpr (DeviceUsed == DeviceType::GPU_CUDA)
            {
                cudnnDataType_t dataType = CUDNN_DATA_FLOAT;
                if constexpr (std::is_same<DataType,float>::value)
                {
                    dataType = CUDNN_DATA_FLOAT;
                }
                else if constexpr (std::is_same<DataType,double>::value)
                {
                    dataType = CUDNN_DATA_DOUBLE;
                }

                unsigned int batchSize = input("Input")->shape()[3];
                unsigned int channelSize = input("Input")->shape()[0];
                unsigned int width = input("Input")->shape()[1];
                unsigned int height = input("Input")->shape()[2];
 
                RUN_CUDNN(cudnnSetPooling2dDescriptor( m_poolingDescriptor,
                                                       CUDNN_POOLING_MAX,
                                                       CUDNN_NOT_PROPAGATE_NAN,
                                                       2,
                                                       2,
                                                       0,
                                                       0,
                                                       2,
                                                       2));

                RUN_CUDNN(cudnnSetTensor4dDescriptor( m_inputTensorDescriptor,
                                                      CUDNN_TENSOR_NHWC,
                                                      dataType,
                                                      batchSize,
                                                      channelSize,
                                                      height, width));

                RUN_CUDNN(cudnnSetTensor4dDescriptor(m_outputTensorDescriptor,
                                                     CUDNN_TENSOR_NHWC,
                                                     dataType,
                                                     batchSize,
                                                     channelSize,
                                                     height/2, width/2));
            }

            return true;
        }

        virtual void evaluate() override
        {
            CHECK_GPU;

            Tensor<DeviceUsed, DataType> *_input = input("Input")->template toType<DataType>();
            Tensor<DeviceUsed, DataType> *_output = output("Output")->template toType<DataType>();

            unsigned int newWidth = _output->shape()[1];
            unsigned int newHeight = _output->shape()[2];
            unsigned int oldWidth = _input->shape()[1];
            unsigned int oldHeight = _input->shape()[2];
            unsigned int batchSize = _output->shape()[3];

            unsigned int depthSize = _input->shape()[0];

            if constexpr (DeviceUsed == DeviceType::CPU_NAIVE )
            {
                Tensor<DeviceUsed, unsigned int> *_switchX = output("SwitchX")->template toType<unsigned int>();
                Tensor<DeviceUsed, unsigned int> *_switchY = output("SwitchY")->template toType<unsigned int>();

                for (unsigned int b = 0; b < batchSize; ++b)
                {
                    for(unsigned int y = 0;y< newHeight; ++y)
                    {
                        for(unsigned int x =0;x<newWidth; ++x)
                        {
                            for(unsigned int depth = 0;depth<depthSize;++depth)
                            {

                                DataType a = (*_input)[(b * oldWidth * oldHeight + y*2*oldWidth + x*2)*depthSize + depth];
                                DataType _b = (*_input)[(b * oldWidth * oldHeight + y*2*oldWidth + x*2 + 1)*depthSize + depth];
                                DataType c = (*_input)[(b * oldWidth * oldHeight + (y*2+1)*oldWidth + x*2)*depthSize + depth];
                                DataType d = (*_input)[(b * oldWidth * oldHeight + (y*2+1)*oldWidth + x*2 + 1)*depthSize +depth];
                        
                                DataType max = a;
                                unsigned int locationX = x*2;
                                unsigned int locationY = y*2;
                                if (_b > max)
                                {
                                    locationX = x*2+1;
                                    locationY = y*2;
                                    max = _b;
                                }

                                if (c > max)
                                {
                                    locationX =x* 2;
                                    locationY = y*2+1;
                                    max = c;
                                }

                                if (d > max)
                                {
                                    locationX = x*2+1;
                                    locationY = y*2+1;
                                    max = d;
                                }

                                (*_output)[(b * newWidth*newHeight + newWidth * y + x)*depthSize + depth] = max;
                                (*_switchX)[(b* newWidth*newHeight + newWidth *y +x)*depthSize + depth] = locationX;
                                (*_switchY)[(b* newWidth*newHeight + newWidth *y +x)*depthSize + depth] = locationY;
                            }
                        }
                    }
                }
            }
            else if constexpr (DeviceUsed == DeviceType::GPU_CUDA)
            {
                DataType alpha = 1.0;
                DataType beta = 0.0;

                RUN_CUDNN(cudnnPoolingForward( Context<DeviceUsed>::getSingleton().cudnnHandle(m_deviceId),
                                               m_poolingDescriptor,
                                               &alpha,
                                               m_inputTensorDescriptor,
                                               _input->gpuDataHandle(),
                                               &beta,
                                               m_outputTensorDescriptor,
                                               _output->gpuDataHandle()));    
            }
        } 
    };
}

#endif
