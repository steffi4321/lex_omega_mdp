#pragma once

#include "storm/storage/jani/Model.h"
#include "storm/storage/jani/Automaton.h"


namespace storm {
    namespace jani {
        class JaniLocationExpander {
        public:
            JaniLocationExpander(Model const& original);
            void transform(std::string const& automatonName, std::string const& variableName);
            Model const& getResult() const;

            // The following variables will be set during executing and should therefore only be used afterwards.

            // Maps each old location index to a map that maps every variable value to the index of the (new) location that corresponds to the old location and variable value
            std::map<uint64_t, std::map<int64_t, uint64_t>> locationVariableValueMap;
            std::vector<storm::expressions::Expression> variableDomain;


        private:
            Model const& original;
            Model newModel;

            Automaton transformAutomaton(Automaton const& automaton, std::string const& variableName,bool useTransientVariables = true);

        };
    }

}
