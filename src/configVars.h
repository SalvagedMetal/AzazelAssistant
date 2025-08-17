#include "model.h"

namespace ConfigVars {
    struct Model {
        std::string name;
        std::string purpose;
        std::string path;
        int ngl;
        int n_ctx;
        float temp;
        float min_p;
        float top_p;
        float typical;
        int top_k;
        float dist;
        std::string init_message;
        bool keepHistory;
    };
};
