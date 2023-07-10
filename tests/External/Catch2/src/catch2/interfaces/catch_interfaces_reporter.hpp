
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#ifndef CATCH_INTERFACES_REPORTER_HPP_INCLUDED
#define CATCH_INTERFACES_REPORTER_HPP_INCLUDED

#include <catch2/catch_section_info.hpp>
#include <catch2/catch_totals.hpp>
#include <catch2/catch_assertion_result.hpp>
#include <catch2/internal/catch_message_info.hpp>
#include <catch2/internal/catch_stringref.hpp>
#include <catch2/internal/catch_unique_ptr.hpp>
#include <catch2/internal/catch_move_and_forward.hpp>
#include <catch2/benchmark/catch_estimate.hpp>
#include <catch2/benchmark/catch_outlier_classification.hpp>


#include <string>
#include <vector>
#include <iosfwd>

namespace Catch {

    struct ReporterDescription;
    struct TagInfo;
    struct TestCaseInfo;
    class TestCaseHandle;
    struct IConfig;

    struct ReporterConfig {
        explicit ReporterConfig( IConfig const* _fullConfig );

        ReporterConfig( IConfig const* _fullConfig, std::ostream& _stream );

        std::ostream& stream() const;
        IConfig const* fullConfig() const;

    private:
        std::ostream* m_stream;
        IConfig const* m_fullConfig;
    };

    struct TestRunInfo {
        TestRunInfo(StringRef _name) : name(_name) {}
        StringRef name;
    };

    struct AssertionStats {
        AssertionStats( AssertionResult const& _assertionResult,
                        std::vector<MessageInfo> const& _infoMessages,
                        Totals const& _totals );

        AssertionStats( AssertionStats const& )              = default;
        AssertionStats( AssertionStats && )                  = default;
        AssertionStats& operator = ( AssertionStats const& ) = delete;
        AssertionStats& operator = ( AssertionStats && )     = delete;

        AssertionResult assertionResult;
        std::vector<MessageInfo> infoMessages;
        Totals totals;
    };

    struct SectionStats {
        SectionStats(   SectionInfo const& _sectionInfo,
                        Counts const& _assertions,
                        double _durationInSeconds,
                        bool _missingAssertions );

        SectionInfo sectionInfo;
        Counts assertions;
        double durationInSeconds;
        bool missingAssertions;
    };

    struct TestCaseStats {
        TestCaseStats(  TestCaseInfo const& _testInfo,
                        Totals const& _totals,
                        std::string const& _stdOut,
                        std::string const& _stdErr,
                        bool _aborting );

        TestCaseInfo const * testInfo;
        Totals totals;
        std::string stdOut;
        std::string stdErr;
        bool aborting;
    };

    struct TestRunStats {
        TestRunStats(   TestRunInfo const& _runInfo,
                        Totals const& _totals,
                        bool _aborting );

        TestRunInfo runInfo;
        Totals totals;
        bool aborting;
    };


    struct BenchmarkInfo {
        std::string name;
        double estimatedDuration;
        int iterations;
        int samples;
        unsigned int resamples;
        double clockResolution;
        double clockCost;
    };

    template <class Duration>
    struct BenchmarkStats {
        BenchmarkInfo info;

        std::vector<Duration> samples;
        Benchmark::Estimate<Duration> mean;
        Benchmark::Estimate<Duration> standardDeviation;
        Benchmark::OutlierClassification outliers;
        double outlierVariance;

        template <typename Duration2>
        operator BenchmarkStats<Duration2>() const {
            std::vector<Duration2> samples2;
            samples2.reserve(samples.size());
            for (auto const& sample : samples) {
                samples2.push_back(Duration2(sample));
            }
            return {
                info,
                CATCH_MOVE(samples2),
                mean,
                standardDeviation,
                outliers,
                outlierVariance,
            };
        }
    };

    //! By setting up its preferences, a reporter can modify Catch2's behaviour
    //! in some regards, e.g. it can request Catch2 to capture writes to
    //! stdout/stderr during test execution, and pass them to the reporter.
    struct ReporterPreferences {
        //! Catch2 should redirect writes to stdout and pass them to the
        //! reporter
        bool shouldRedirectStdOut = false;
        //! Catch2 should call `Reporter::assertionEnded` even for passing
        //! assertions
        bool shouldReportAllAssertions = false;
    };


    struct IStreamingReporter {
    protected:
        //! Derived classes can set up their preferences here
        ReporterPreferences m_preferences;
        //! The test run's config as filled in from CLI and defaults
        IConfig const* m_config;

    public:
        IStreamingReporter( IConfig const* config ): m_config( config ) {}

        virtual ~IStreamingReporter() = default;

        // Implementing class must also provide the following static methods:
        // static std::string getDescription();

        ReporterPreferences const& getPreferences() const {
            return m_preferences;
        }

        virtual void noMatchingTestCases( std::string const& spec ) = 0;

        virtual void reportInvalidArguments(std::string const&) {}

        virtual void testRunStarting( TestRunInfo const& testRunInfo ) = 0;

        //! Called _once_ for each TEST_CASE, no matter how many times it is entered
        virtual void testCaseStarting( TestCaseInfo const& testInfo ) = 0;
        //! Called _every time_ a TEST_CASE is entered, including repeats (due to sections)
        virtual void testCasePartialStarting( TestCaseInfo const& testInfo, uint64_t partNumber ) = 0;
        virtual void sectionStarting( SectionInfo const& sectionInfo ) = 0;

        virtual void benchmarkPreparing( StringRef ) {}
        virtual void benchmarkStarting( BenchmarkInfo const& ) {}
        virtual void benchmarkEnded( BenchmarkStats<> const& ) {}
        virtual void benchmarkFailed( StringRef ) {}

        virtual void assertionStarting( AssertionInfo const& assertionInfo ) = 0;

        // The return value indicates if the messages buffer should be cleared:
        virtual bool assertionEnded( AssertionStats const& assertionStats ) = 0;

        virtual void sectionEnded( SectionStats const& sectionStats ) = 0;
        //! Called _every time_ a TEST_CASE is entered, including repeats (due to sections)
        virtual void testCasePartialEnded(TestCaseStats const& testCaseStats, uint64_t partNumber ) = 0;
        //! Called _once_ for each TEST_CASE, no matter how many times it is entered
        virtual void testCaseEnded( TestCaseStats const& testCaseStats ) = 0;
        virtual void testRunEnded( TestRunStats const& testRunStats ) = 0;

        virtual void skipTest( TestCaseInfo const& testInfo ) = 0;

        // Default empty implementation provided
        virtual void fatalErrorEncountered( StringRef name );

        //! Writes out information about provided reporters using reporter-specific format
        virtual void listReporters(std::vector<ReporterDescription> const& descriptions) = 0;
        //! Writes out information about provided tests using reporter-specific format
        virtual void listTests(std::vector<TestCaseHandle> const& tests) = 0;
        //! Writes out information about the provided tags using reporter-specific format
        virtual void listTags(std::vector<TagInfo> const& tags) = 0;

    };
    using IStreamingReporterPtr = Detail::unique_ptr<IStreamingReporter>;

} // end namespace Catch

#endif // CATCH_INTERFACES_REPORTER_HPP_INCLUDED
