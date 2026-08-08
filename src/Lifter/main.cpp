#include <exception>
#include <iostream>

#include "Lifter/Cli/CliApplication.hpp"
#include "Lifter/Composition/CompositionRoot.hpp"

int main(int argumentCount, char** argumentValues)
{
    try
    {
        Lifter::Composition::CompositionRoot compositionRoot;

        const Lifter::Cli::CliApplication application(compositionRoot.BinaryProcessor(), compositionRoot.ConfigLoader(),
                                                      std::cout, std::cerr);

        return application.Run(argumentCount, argumentValues);
    }
    catch (const std::exception& error)
    {
        std::cerr << "fatal: " << error.what() << "\n";
        return 1;
    }
}
