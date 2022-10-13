#pragma once
#include "Panel.h"

namespace IFCS
{
    class ModelGenerator : public Panel
    {
    public:
        MAKE_SINGLETON(ModelGenerator)
    protected:
        void RenderContent() override;
    private:
        char Script[1024 * 16];
        std::string Exec(const char* cmd);
        bool RunSomething = false;
        std::string RunResult;
    };
}
