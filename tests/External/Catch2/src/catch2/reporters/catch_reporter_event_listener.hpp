
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#ifndef CATCH_REPORTER_EVENT_LISTENER_HPP_INCLUDED
#define CATCH_REPORTER_EVENT_LISTENER_HPP_INCLUDED

#include <catch2/interfaces/catch_interfaces_reporter.hpp>

namespace Catch {

    /**
     * Base class identifying listeners.
     *
     * Provides default implementation for all IStreamingReporter member
     * functions, so that listeners implementations can pick which
     * member functions it actually cares about.
     */
    class EventListenerBase : public IStreamingReporter {
    public:
        EventListenerBase( ReporterConfig const& config ):
            IStreamingReporter( config.fullConfig() ) {}

        void assertionStarting( AssertionInfo const& assertionInfo ) override;
        bool assertionEnded( AssertionStats const& assertionStats ) override;

        void listReporters(
            std::vector<ReporterDescription> const& descriptions ) override;
        void listTests( std::vector<TestCaseHandle> const& tests ) override;
        void listTags( std::vector<TagInfo> const& tagInfos ) override;

        void noMatchingTestCases( std::string const& spec ) override;
        void testRunStarting( TestRunInfo const& testRunInfo ) override;
        void testCaseStarting( TestCaseInfo const& testInfo ) override;
        void testCasePartialStarting( TestCaseInfo const& testInfo,
                                      uint64_t partNumber ) override;
        void sectionStarting( SectionInfo const& sectionInfo ) override;
        void sectionEnded( SectionStats const& sectionStats ) override;
        void testCasePartialEnded( TestCaseStats const& testCaseStats,
                                   uint64_t partNumber ) override;
        void testCaseEnded( TestCaseStats const& testCaseStats ) override;
        void testRunEnded( TestRunStats const& testRunStats ) override;
        void skipTest( TestCaseInfo const& testInfo ) override;

    };

} // end namespace Catch

#endif // CATCH_REPORTER_EVENT_LISTENER_HPP_INCLUDED
