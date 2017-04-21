#ifndef MODEL_H
#define MODEL_H
#include <cmath>
#include "../DeviceSelection.h"
#include "../Context/Context.h"
#include <string>
#include <map>
#include <utility>
#include <variant>
#include <any>
#include "TensorDescriptor.h"
#include "OperatorDescriptor.h"
#include "Solver.h"


namespace FreeWill
{
    class Model
    {




    private:
        Model();
        Model(const Model &) = delete;
        Model& operator=(const Model &) = delete;

        std::map<TensorDescriptorHandle, TensorDescriptor*> m_tensors;
        std::map<std::string, OperatorDescriptor*> m_operators;


    public:
        static Model* create();
        ~Model();
        bool init(const Solver &solver);
        TensorDescriptorHandle addTensor(const std::string &name, const Shape &shape, bool isBatchTensor = false, DataType dataType = FLOAT);
        int addOperator(const std::string &name,
                        const std::string &operatorName,
                        const std::map<std::string, TensorDescriptorHandle> &inputs,
                        const std::map<std::string, TensorDescriptorHandle> &outputs,
                        const std::map<std::string, std::any> &properties = {}, DataType dataType = FLOAT);
        int addOperator(const std::string &name,
                        FreeWill::OperatorName operatorName,
                        const std::map<std::string, TensorDescriptorHandle> &inputs,
                        const std::map<std::string, TensorDescriptorHandle> &outputs,
                        const std::map<std::string, std::any> &properties = {}, DataType DataType = FLOAT);
    };
}

#endif
