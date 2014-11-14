#ifndef _ROSEX_TAC_H
#define _ROSEX_TAC_H

// tac.h
// 2/6/2013 jichi
//
// Represent the three-address-code.

#include <string>
#include <list>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include "xt/c++11.h"

class SgNode;
class TacTablePrivate;

///  Representation of the data flow table
class TacTable
{
  friend class TacTablePrivate;
  typedef TacTablePrivate D;

  D *d_; // Pimp private data pointer

  // - Types -
protected:
  ///  Reference of a symbol
  struct Reference
  {
    ///  Original node in the source AST
    SgNode *source;

    /**
     *  \brief  The symbolic representation of the source, built in symbolic.h
     *
     *  Determine if the two references are the same or not.
     */
    std::string symbol;

    explicit Reference(SgNode *_source = nullptr)
      : source(_source) { }
    explicit Reference(const std::string &_symbol, SgNode *_source = nullptr)
      : source(_source), symbol(_symbol) { }
    Reference(const Reference &that)
      : source(that.source), symbol(that.symbol) { }

    ///  Create symbolic reference.
    static Reference createSymbol(SgNode *src);

    ///  Create temporary reference
    static Reference createTemp(SgNode *src = nullptr);

    ///  Return if this reference is a temporary variable
    bool isTemporary() const;

    ///  Unparse the symbol to string
    std::string toString() const;
  };
  typedef std::list<Reference> ReferenceList;

  ///  Table entry, represents an assignment operation to a reference
  struct Entry
  {
    ///  Orinal source node of this operation
    SgNode *source;

    ///  Orinal source node of this operation
    SgNode *resultSource;

    ///  The reference being modified by the operation
    Reference result;

    ///  The reference being modified by the operation
    ReferenceList operands;

    explicit Entry(SgNode *_source = nullptr)
      : source(_source), resultSource(nullptr) {}

    Entry(const Entry &that)
      : source(that.source), resultSource(that.resultSource), result(that.result), operands(that.operands) {}

    ///  Unparse to string
    std::string toString() const;
  };
  typedef std::list<Entry> EntryList;

  // - Constructions -
public:
  explicit TacTable(SgNode *block = nullptr);
  ~TacTable();

  ///  Initialize with a SgBasicBlock
  void initWithBlock(SgNode *block);

  /**
   *  \brief  Initialize with a list of SgStatement
   *
   *
   *  It is assuming that the input statements are from a basic block.
   *  i.e., there is not branches, no loops, no procedure calls.
   *  Only two kinds of statements will be analyzed at the moment:
   *  - Assignment in expression statement
   *  - Assignment in declaration statement
   *  Other statements will be ignored.
   */
  template <typename L>
    void initWithStatements(const L &l)
    {
      clear();
      for (BOOST_AUTO(p, l.begin()); p != l.end(); ++p)
        addStatement(*p);
    }

  ///  Add statement into the dataflow table. Return if the table is modified.
  void addStatement(SgNode *stmt);

  ///  Clear the table
  void clear();

  ///  Return if the table is empty
  bool isEmpty() const;

  ///  Unparse to string
  std::string toString() const;
};


#endif // _ROSEX_TAC_H
