
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/reporters/catch_reporter_junit.hpp>

#include <catch2/reporters/catch_reporter_helpers.hpp>
#include <catch2/catch_tostring.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/internal/catch_textflow.hpp>
#include <catch2/interfaces/catch_interfaces_config.hpp>
#include <catch2/catch_test_case_info.hpp>

#include <cassert>
#include <ctime>
#include <algorithm>

namespace Catch {

    namespace {
        std::string getCurrentTimestamp() {
            time_t rawtime;
            std::time(&rawtime);

            std::tm timeInfo = {};
#if defined (_MSC_VER) || defined (__MINGW32__)
            gmtime_s(&timeInfo, &rawtime);
#else
            gmtime_r(&rawtime, &timeInfo);
#endif

            auto const timeStampSize = sizeof("2017-01-16T17:06:45Z");
            char timeStamp[timeStampSize];
            const char * const fmt = "%Y-%m-%dT%H:%M:%SZ";

            std::strftime(timeStamp, timeStampSize, fmt, &timeInfo);

            return std::string(timeStamp, timeStampSize - 1);
        }

        std::string fileNameTag(std::vector<Tag> const& tags) {
            auto it = std::find_if(begin(tags),
                                   end(tags),
                                   [] (Tag const& tag) {
                                       return tag.original.size() > 0
                                           && tag.original[0] == '#'; });
            if (it != tags.end()) {
                return static_cast<std::string>(
                    it->original.substr(1, it->original.size() - 1)
                );
            }
            return std::string();
        }
    } // anonymous namespace

    JunitReporter::JunitReporter( ReporterConfig const& _config )
        :   CumulativeReporterBase( _config ),
            xml( _config.stream() )
        {
            m_preferences.shouldRedirectStdOut = true;
            m_preferences.shouldReportAllAssertions = true;
        }

    JunitReporter::~JunitReporter() {}

    std::string JunitReporter::getDescription() {
        return "Reports test results in an XML format that looks like Ant's junitreport target";
    }

    void JunitReporter::noMatchingTestCases( std::string const& /*spec*/ ) {}

    void JunitReporter::testRunStarting( TestRunInfo const& runInfo )  {
        CumulativeReporterBase::testRunStarting( runInfo );
        xml.startElement( "testsuites" );
        suiteTimer.start();
        stdOutForSuite.clear();
        stdErrForSuite.clear();
        unexpectedExceptions = 0;
    }

    void JunitReporter::testCaseStarting( TestCaseInfo const& testCaseInfo ) {
        m_okToFail = testCaseInfo.okToFail();
    }

    bool JunitReporter::assertionEnded( AssertionStats const& assertionStats ) {
        if( assertionStats.assertionResult.getResultType() == ResultWas::ThrewException && !m_okToFail )
            unexpectedExceptions++;
        return CumulativeReporterBase::assertionEnded( assertionStats );
    }

    void JunitReporter::testCaseEnded( TestCaseStats const& testCaseStats ) {
        stdOutForSuite += testCaseStats.stdOut;
        stdErrForSuite += testCaseStats.stdErr;
        CumulativeReporterBase::testCaseEnded( testCaseStats );
    }

    void JunitReporter::testRunEndedCumulative() {
        const auto suiteTime = suiteTimer.getElapsedSeconds();
        writeRun( *m_testRun, suiteTime );
        xml.endElement();
    }

    void JunitReporter::writeRun( TestRunNode const& testRunNode, double suiteTime ) {
        XmlWriter::ScopedElement e = xml.scopedElement( "testsuite" );

        TestRunStats const& stats = testRunNode.value;
        xml.writeAttribute( "name"_sr, stats.runInfo.name );
        xml.writeAttribute( "errors"_sr, unexpectedExceptions );
        xml.writeAttribute( "failures"_sr, stats.totals.assertions.failed-unexpectedExceptions );
        xml.writeAttribute( "tests"_sr, stats.totals.assertions.total() );
        xml.writeAttribute( "hostname"_sr, "tbd"_sr ); // !TBD
        if( m_config->showDurations() == ShowDurations::Never )
            xml.writeAttribute( "time"_sr, ""_sr );
        else
            xml.writeAttribute( "time"_sr, suiteTime );
        xml.writeAttribute( "timestamp"_sr, getCurrentTimestamp() );

        // Write properties if there are any
        if (m_config->hasTestFilters() || m_config->rngSeed() != 0) {
            auto properties = xml.scopedElement("properties");
            if (m_config->hasTestFilters()) {
                xml.scopedElement("property")
                    .writeAttribute("name"_sr, "filters"_sr)
                    .writeAttribute("value"_sr, serializeFilters(m_config->getTestsOrTags()));
            }
            if (m_config->rngSeed() != 0) {
                xml.scopedElement("property")
                    .writeAttribute("name"_sr, "random-seed"_sr)
                    .writeAttribute("value"_sr, m_config->rngSeed());
            }
        }

        // Write test cases
        for( auto const& child : testRunNode.children )
            writeTestCase( *child );

        xml.scopedElement( "system-out" ).writeText( trim( stdOutForSuite ), XmlFormatting::Newline );
        xml.scopedElement( "system-err" ).writeText( trim( stdErrForSuite ), XmlFormatting::Newline );
    }

    void JunitReporter::writeTestCase( TestCaseNode const& testCaseNode ) {
        TestCaseStats const& stats = testCaseNode.value;

        // All test cases have exactly one section - which represents the
        // test case itself. That section may have 0-n nested sections
        assert( testCaseNode.children.size() == 1 );
        SectionNode const& rootSection = *testCaseNode.children.front();

        std::string className = stats.testInfo->className;

        if( className.empty() ) {
            className = fileNameTag(stats.testInfo->tags);
            if ( className.empty() )
                className = "global";
        }

        if ( !m_config->name().empty() )
            className = m_config->name() + "." + className;

        writeSection( className, "", rootSection );
    }

    void JunitReporter::writeSection(  std::string const& className,
                        std::string const& rootName,
                        SectionNode const& sectionNode ) {
        std::string name = trim( sectionNode.stats.sectionInfo.name );
        if( !rootName.empty() )
            name = rootName + '/' + name;

        if( !sectionNode.assertions.empty() ||
            !sectionNode.stdOut.empty() ||
            !sectionNode.stdErr.empty() ) {
            XmlWriter::ScopedElement e = xml.scopedElement( "testcase" );
            if( className.empty() ) {
                xml.writeAttribute( "classname"_sr, name );
                xml.writeAttribute( "name"_sr, "root"_sr );
            }
            else {
                xml.writeAttribute( "classname"_sr, className );
                xml.writeAttribute( "name"_sr, name );
            }
            xml.writeAttribute( "time"_sr, ::Catch::Detail::stringify( sectionNode.stats.durationInSeconds ) );
            // This is not ideal, but it should be enough to mimic gtest's
            // junit output.
            // Ideally the JUnit reporter would also handle `skipTest`
            // events and write those out appropriately.
            xml.writeAttribute( "status"_sr, "run"_sr );

            writeAssertions( sectionNode );

            if( !sectionNode.stdOut.empty() )
                xml.scopedElement( "system-out" ).writeText( trim( sectionNode.stdOut ), XmlFormatting::Newline );
            if( !sectionNode.stdErr.empty() )
                xml.scopedElement( "system-err" ).writeText( trim( sectionNode.stdErr ), XmlFormatting::Newline );
        }
        for( auto const& childNode : sectionNode.childSections )
            if( className.empty() )
                writeSection( name, "", *childNode );
            else
                writeSection( className, name, *childNode );
    }

    void JunitReporter::writeAssertions( SectionNode const& sectionNode ) {
        for( auto const& assertion : sectionNode.assertions )
            writeAssertion( assertion );
    }

    void JunitReporter::writeAssertion( AssertionStats const& stats ) {
        AssertionResult const& result = stats.assertionResult;
        if( !result.isOk() ) {
            std::string elementName;
            switch( result.getResultType() ) {
                case ResultWas::ThrewException:
                case ResultWas::FatalErrorCondition:
                    elementName = "error";
                    break;
                case ResultWas::ExplicitFailure:
                case ResultWas::ExpressionFailed:
                case ResultWas::DidntThrowException:
                    elementName = "failure";
                    break;

                // We should never see these here:
                case ResultWas::Info:
                case ResultWas::Warning:
                case ResultWas::Ok:
                case ResultWas::Unknown:
                case ResultWas::FailureBit:
                case ResultWas::Exception:
                    elementName = "internalError";
                    break;
            }

            XmlWriter::ScopedElement e = xml.scopedElement( elementName );

            xml.writeAttribute( "message"_sr, result.getExpression() );
            xml.writeAttribute( "type"_sr, result.getTestMacroName() );

            ReusableStringStream rss;
            if (stats.totals.assertions.total() > 0) {
                rss << "FAILED" << ":\n";
                if (result.hasExpression()) {
                    rss << "  ";
                    rss << result.getExpressionInMacro();
                    rss << '\n';
                }
                if (result.hasExpandedExpression()) {
                    rss << "with expansion:\n";
                    rss << TextFlow::Column(result.getExpandedExpression()).indent(2) << '\n';
                }
            } else {
                rss << '\n';
            }

            if( !result.getMessage().empty() )
                rss << result.getMessage() << '\n';
            for( auto const& msg : stats.infoMessages )
                if( msg.type == ResultWas::Info )
                    rss << msg.message << '\n';

            rss << "at " << result.getSourceInfo();
            xml.writeText( rss.str(), XmlFormatting::Newline );
        }
    }

} // end namespace Catch
