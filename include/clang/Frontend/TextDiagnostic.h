//===--- TextDiagnostic.h - Text Diagnostic Pretty-Printing -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a utility class that provides support for textual pretty-printing of
// diagnostics. It is used to implement the different code paths which require
// such functionality in a consistent way.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_TEXT_DIAGNOSTIC_H_
#define LLVM_CLANG_FRONTEND_TEXT_DIAGNOSTIC_H_

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {
class DiagnosticOptions;
class LangOptions;
class SourceManager;

/// \brief Class to encapsulate the logic for formatting and printing a textual
/// diagnostic message.
///
/// This class provides an interface for building and emitting a textual
/// diagnostic, including all of the macro backtraces, caret diagnostics, FixIt
/// Hints, and code snippets. In the presence of macros this involves
/// a recursive process, synthesizing notes for each macro expansion.
///
/// The purpose of this class is to isolate the implementation of printing
/// beautiful text diagnostics from any particular interfaces. The Clang
/// DiagnosticClient is implemented through this class as is diagnostic
/// printing coming out of libclang.
///
/// A brief worklist:
/// FIXME: Sink the recursive printing of template instantiations into this
/// class.
class TextDiagnostic {
  raw_ostream &OS;
  const SourceManager &SM;
  const LangOptions &LangOpts;
  const DiagnosticOptions &DiagOpts;

  /// \brief The location of the previous diagnostic if known.
  ///
  /// This will be invalid in cases where there is no (known) previous
  /// diagnostic location, or that location itself is invalid or comes from
  /// a different source manager than SM.
  SourceLocation LastLoc;

  /// \brief The location of the last include whose stack was printed if known.
  ///
  /// Same restriction as \see LastLoc essentially, but tracking include stack
  /// root locations rather than diagnostic locations.
  SourceLocation LastIncludeLoc;

  /// \brief The level of the last diagnostic emitted.
  ///
  /// The level of the last diagnostic emitted. Used to detect level changes
  /// which change the amount of information displayed.
  DiagnosticsEngine::Level LastLevel;

public:
  TextDiagnostic(raw_ostream &OS,
                 const SourceManager &SM,
                 const LangOptions &LangOpts,
                 const DiagnosticOptions &DiagOpts,
                 FullSourceLoc LastLoc = FullSourceLoc(),
                 FullSourceLoc LastIncludeLoc = FullSourceLoc(),
                 DiagnosticsEngine::Level LastLevel
                   = DiagnosticsEngine::Level());

  /// \brief Get the last diagnostic location emitted.
  SourceLocation getLastLoc() const { return LastLoc; }

  /// \brief Get the last emitted include stack location.
  SourceLocation getLastIncludeLoc() const { return LastIncludeLoc; }

  /// \brief Get the last diagnostic level.
  DiagnosticsEngine::Level getLastLevel() const { return LastLevel; }

  void Emit(SourceLocation Loc, DiagnosticsEngine::Level Level,
            StringRef Message, ArrayRef<CharSourceRange> Ranges,
            ArrayRef<FixItHint> FixItHints,
            bool LastCaretDiagnosticWasNote = false);

  /// \brief Emit the caret and underlining text.
  ///
  /// Walks up the macro expansion stack printing the code snippet, caret,
  /// underlines and FixItHint display as appropriate at each level. Walk is
  /// accomplished by calling itself recursively.
  ///
  /// FIXME: Remove macro expansion from this routine, it shouldn't be tied to
  /// caret diagnostics.
  /// FIXME: Break up massive function into logical units.
  ///
  /// \param Loc The location for this caret.
  /// \param Ranges The underlined ranges for this code snippet.
  /// \param Hints The FixIt hints active for this diagnostic.
  /// \param MacroSkipEnd The depth to stop skipping macro expansions.
  /// \param OnMacroInst The current depth of the macro expansion stack.
  void EmitCaret(SourceLocation Loc,
                 SmallVectorImpl<CharSourceRange>& Ranges,
                 ArrayRef<FixItHint> Hints,
                 unsigned &MacroDepth,
                 unsigned OnMacroInst = 0);

  /// \brief Emit a code snippet and caret line.
  ///
  /// This routine emits a single line's code snippet and caret line..
  ///
  /// \param Loc The location for the caret.
  /// \param Ranges The underlined ranges for this code snippet.
  /// \param Hints The FixIt hints active for this diagnostic.
  void EmitSnippetAndCaret(SourceLocation Loc,
                           SmallVectorImpl<CharSourceRange>& Ranges,
                           ArrayRef<FixItHint> Hints);

  /// \brief Print the diagonstic level to a raw_ostream.
  ///
  /// This is a static helper that handles colorizing the level and formatting
  /// it into an arbitrary output stream. This is used internally by the
  /// TextDiagnostic emission code, but it can also be used directly by
  /// consumers that don't have a source manager or other state that the full
  /// TextDiagnostic logic requires.
  static void printDiagnosticLevel(raw_ostream &OS,
                                   DiagnosticsEngine::Level Level,
                                   bool ShowColors);

  /// \brief Pretty-print a diagnostic message to a raw_ostream.
  ///
  /// This is a static helper to handle the line wrapping, colorizing, and
  /// rendering of a diagnostic message to a particular ostream. It is
  /// publically visible so that clients which do not have sufficient state to
  /// build a complete TextDiagnostic object can still get consistent
  /// formatting of their diagnostic messages.
  ///
  /// \param OS Where the message is printed
  /// \param Level Used to colorizing the message
  /// \param Message The text actually printed
  /// \param CurrentColumn The starting column of the first line, accounting
  ///                      for any prefix.
  /// \param Columns The number of columns to use in line-wrapping, 0 disables
  ///                all line-wrapping.
  /// \param ShowColors Enable colorizing of the message.
  static void printDiagnosticMessage(raw_ostream &OS,
                                     DiagnosticsEngine::Level Level,
                                     StringRef Message,
                                     unsigned CurrentColumn, unsigned Columns,
                                     bool ShowColors);

private:
  void emitIncludeStack(SourceLocation Loc, DiagnosticsEngine::Level Level);
  void emitIncludeStackRecursively(SourceLocation Loc);
  void EmitDiagnosticLoc(SourceLocation Loc, PresumedLoc PLoc,
                         DiagnosticsEngine::Level Level,
                         ArrayRef<CharSourceRange> Ranges);
  void HighlightRange(const CharSourceRange &R,
                      unsigned LineNo, FileID FID,
                      const std::string &SourceLine,
                      std::string &CaretLine);
  std::string BuildFixItInsertionLine(unsigned LineNo,
                                      const char *LineStart,
                                      const char *LineEnd,
                                      ArrayRef<FixItHint> Hints);
  void ExpandTabs(std::string &SourceLine, std::string &CaretLine);
  void EmitParseableFixits(ArrayRef<FixItHint> Hints);
};

} // end namespace clang

#endif