//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Implementation of normal form game classes
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#include <iostream>
#include "libgambit.h"

//===========================================================================
//                          class gbtNfgOutcome
//===========================================================================

//---------------------------------------------------------------------------
//                               Lifecycle
//---------------------------------------------------------------------------

gbtNfgOutcomeRep::gbtNfgOutcomeRep(int n, gbtNfgGameRep *N)
  : number(n), nfg(N),
    m_textPayoffs(nfg->NumPlayers()), 
    m_ratPayoffs(nfg->NumPlayers()),
    m_doublePayoffs(nfg->NumPlayers())
{ 
  for (int pl = 1; pl <= m_textPayoffs.Length(); pl++) {
    m_textPayoffs[pl] = "0";
    m_doublePayoffs[pl] = 0.0;
  }
}

//---------------------------------------------------------------------------
//                              Data access
//---------------------------------------------------------------------------

void gbtNfgOutcomeRep::SetPayoff(int pl, const std::string &p_value)
{
  m_textPayoffs[pl] = p_value;
  m_ratPayoffs[pl] = ToRational(p_value);
  m_doublePayoffs[pl] = (double) m_ratPayoffs[pl];
}

//===========================================================================
//                          class gbtNfgPlayer
//===========================================================================

//---------------------------------------------------------------------------
//                               Lifecycle
//---------------------------------------------------------------------------

gbtNfgPlayerRep::gbtNfgPlayerRep(int n, gbtNfgGameRep *N, int num)
  : number(n), m_nfg(N), strategies(num)
{ 
  for (int j = 1; j <= num; j++) {
    strategies[j] = new gbtNfgStrategyRep(this);
    strategies[j]->m_number = 1;
  }
}

gbtNfgPlayerRep::~gbtNfgPlayerRep()
{ 
  for (int j = 1; j <= strategies.Length(); strategies[j++]->Invalidate());
}

//---------------------------------------------------------------------------
//                               Strategies
//---------------------------------------------------------------------------

gbtNfgStrategy gbtNfgPlayerRep::NewStrategy(void)
{
  gbtNfgStrategyRep *strategy = new gbtNfgStrategyRep(this);
  strategies.Append(strategy);
  strategy->m_number = strategies.Length();
  strategy->m_index = -1;   // this flags this action as new
  m_nfg->RebuildTable();
  return strategy;
}

void gbtNfgStrategyRep::DeleteStrategy(void)
{
  if (m_player->NumStrats() == 1)  return;

  m_player->strategies.Remove(m_player->strategies.Find(this));
  for (int st = 1; st <= m_player->strategies.Length(); st++) {
    m_player->strategies[st]->m_number = st;
  }
  m_player->m_nfg->RebuildTable();
  this->Invalidate();
}

gbtNfgPlayer gbtNfgStrategyRep::GetPlayer(void) const
{ return m_player; }

//===========================================================================
//                            class gbtNfgGame
//===========================================================================

//---------------------------------------------------------------------------
//                               Lifecycle
//---------------------------------------------------------------------------

/// This convenience function computes the Cartesian product of the
/// elements in dim.
static int Product(const gbtArray<int> &dim)
{
  int accum = 1;
  for (int i = 1; i <= dim.Length(); accum *= dim[i++]);
  return accum;
}
  
gbtNfgGameRep::gbtNfgGameRep(const gbtArray<int> &dim)
  : m_title("Untitled strategic game"),
    dimensions(dim), players(dim.Length()),
    results(Product(dim)), efg(0)
{
  for (int pl = 1; pl <= players.Length(); pl++)  {
    players[pl] = new gbtNfgPlayerRep(pl, this, dim[pl]);
    players[pl]->name = ToText(pl);
    for (int st = 1; st <= players[pl]->NumStrats(); st++)
      players[pl]->strategies[st]->m_name = ToText(st);
  }
  IndexStrategies();

  for (int cont = 1; cont <= results.Length();
       results[cont++] = 0);
}

gbtNfgGameRep::~gbtNfgGameRep()
{
  for (int pl = 1; pl <= players.Length(); players[pl++]->Invalidate());
  for (int outc = 1; outc <= outcomes.Length(); outcomes[outc++]->Invalidate());
}

//---------------------------------------------------------------------------
//                          General data access
//---------------------------------------------------------------------------

bool gbtNfgGameRep::IsConstSum(void) const
{
  gbtNfgContingencyIterator iter(gbtNfgSupport(const_cast<gbtNfgGameRep *>(this)));

  gbtRational sum(0);
  for (int pl = 1; pl <= players.Length(); pl++) {
    sum += iter.GetPayoff(pl);
  }

  while (iter.NextContingency()) {
    gbtRational newsum(0);
    for (int pl = 1; pl <= players.Length(); pl++) {
      newsum += iter.GetPayoff(pl);
    }

    if (newsum != sum) {
      return false;
    }
  }

  return true;
}

gbtRational gbtNfgGameRep::GetMinPayoff(int player) const
{
  int index, p, p1, p2;
  
  if (outcomes.Length() == 0)  return gbtRational(0);

  if (player) {
    p1 = p2 = player;
  }
  else {
    p1 = 1;
    p2 = players.Length();
  }
  
  gbtRational minpay = outcomes[1]->GetPayoff(p1);
  for (index = 1; index <= outcomes.Length(); index++)  {
    for (p = p1; p <= p2; p++) {
      if (outcomes[index]->GetPayoff(p) < minpay) {
	minpay = outcomes[index]->GetPayoff(p);
      }
    }
  }
  return minpay;
}

gbtRational gbtNfgGameRep::GetMaxPayoff(int player) const
{
  int index, p, p1, p2;

  if (outcomes.Length() == 0)  return gbtRational(0);

  if (player) {
    p1 = p2 = player;
  }
  else {
    p1 = 1;
    p2 = players.Length();
  }

  gbtRational maxpay = outcomes[1]->GetPayoff(p1);
  for (index = 1; index <= outcomes.Length(); index++)  {
    for (p = p1; p <= p2; p++)
      if (outcomes[index]->GetPayoff(p) > maxpay)
	maxpay = outcomes[index]->GetPayoff(p);
  }
  return maxpay;
}

//---------------------------------------------------------------------------
//                          Writing data files
//---------------------------------------------------------------------------

/// This convenience function constructs a copy of the string, with
/// any explicit quotes preceded by a backslash
static std::string EscapeQuotes(const std::string &s)
{
  std::string ret;
  
  for (unsigned int i = 0; i < s.length(); i++)  {
    if (s[i] == '"')   ret += '\\';
    ret += s[i];
  }

  return ret;
}

void gbtNfgGameRep::WriteNfgFile(std::ostream &p_file) const
{ 
  p_file << "NFG 1 R";
  p_file << " \"" << EscapeQuotes(GetTitle()) << "\" { ";

  for (int i = 1; i <= NumPlayers(); i++)
    p_file << '"' << EscapeQuotes(GetPlayer(i)->GetName()) << "\" ";

  p_file << "}\n\n{ ";
  
  for (int i = 1; i <= NumPlayers(); i++)   {
    gbtNfgPlayerRep *player = GetPlayer(i);
    p_file << "{ ";
    for (int j = 1; j <= player->NumStrats(); j++)
      p_file << '"' << EscapeQuotes(player->GetStrategy(j)->GetName()) << "\" ";
    p_file << "}\n";
  }
  
  p_file << "}\n";

  p_file << "\"" << EscapeQuotes(m_comment) << "\"\n\n";

  int ncont = 1;
  for (int i = 1; i <= NumPlayers(); i++)
    ncont *= NumStrats(i);

  p_file << "{\n";
  for (int outc = 1; outc <= outcomes.Length(); outc++)   {
    p_file << "{ \"" << EscapeQuotes(outcomes[outc]->name) << "\" ";
    for (int pl = 1; pl <= players.Length(); pl++)  {
      p_file << outcomes[outc]->m_textPayoffs[pl];

      if (pl < players.Length())
	p_file << ", ";
      else
	p_file << " }\n";
    }
  }
  p_file << "}\n";
  
  for (int cont = 1; cont <= ncont; cont++)  {
    if (results[cont] != 0)
      p_file << results[cont]->number << ' ';
    else
      p_file << "0 ";
  }

  p_file << '\n';
}

//---------------------------------------------------------------------------
//                               Players
//---------------------------------------------------------------------------

gbtNfgPlayer gbtNfgGameRep::NewPlayer(void)
{
  gbtNfgPlayerRep *player = new gbtNfgPlayerRep(players.Length() + 1, this, 1);
  players.Append(player);
  dimensions.Append(1);

  for (int outc = 1; outc <= outcomes.Length(); outc++) {
    outcomes[outc]->m_textPayoffs.Append("0");
    outcomes[outc]->m_ratPayoffs.Append(0);
    outcomes[outc]->m_doublePayoffs.Append(0.0);
  }

  return player;
}

//---------------------------------------------------------------------------
//                               Strategies
//---------------------------------------------------------------------------

int gbtNfgGameRep::NumStrats(int pl) const
{
  return players[pl]->strategies.Length();
}

int gbtNfgGameRep::ProfileLength(void) const
{
  int nprof = 0;
  for (int i = 1; i <= players.Length();
       nprof += players[i++]->strategies.Length());
  return nprof;
}

//---------------------------------------------------------------------------
//                               Outcomes
//---------------------------------------------------------------------------

/// Creates a new outcome in the strategic game.  By default, all
/// payoffs to players are set to zero.  Returns the newly created outcome.
gbtNfgOutcome gbtNfgGameRep::NewOutcome(void)
{
  gbtNfgOutcomeRep *outcome = new gbtNfgOutcomeRep(outcomes.Length() + 1, this);
  outcomes.Append(outcome);
  return outcome;
}

/// Deletes an outcome from the strategic game.  If the outcome appears
/// in any contingency of the game, the outcome of those contingencies
/// are reset to the trivial null outcome.
void gbtNfgGameRep::DeleteOutcome(gbtNfgOutcome outcome)
{
  for (int i = 1; i <= results.Length(); i++) {
    if (results[i] == outcome)
      results[i] = 0;
  }

  outcomes.Remove(outcome->GetNumber())->Invalidate();

  for (int outc = 1; outc <= outcomes.Length(); outc++)
    outcomes[outc]->number = outc;
}

//---------------------------------------------------------------------------
//                         Private member functions
//---------------------------------------------------------------------------

void gbtNfgGameRep::IndexStrategies(void)
{
  long offset = 1L;

  for (int i = 1; i <= NumPlayers(); i++)  {
    int j;
    for (j = 1; j <= NumStrats(i); j++)  {
      gbtNfgStrategyRep *s = (players[i])->strategies[j];
      s->m_number = j;
      s->m_index = (j - 1) * offset;
    }
    offset *= (j - 1);
  }
}

/// This rebuilds a new table of outcomes after the game has been
/// redimensioned (change in the number of strategies).  Strategies
/// numbered -1 are identified as the new strategies.
void gbtNfgGameRep::RebuildTable(void)
{
  long size = 1L;
  gbtArray<long> offsets(players.Length());
  for (int pl = 1; pl <= players.Length(); pl++) {
    offsets[pl] = size;
    size *= players[pl]->NumStrats();
  }

  gbtArray<gbtNfgOutcomeRep *> newResults(size);
  for (int i = 1; i <= newResults.Length(); newResults[i++] = 0);

  gbtNfgContingencyIterator iter(gbtNfgSupport(const_cast<gbtNfgGameRep *>(this)));

  do {
    long newindex = 0L;
    for (int pl = 1; pl <= players.Length(); pl++) {
      if (iter.profile.GetStrategy(pl)->m_index < 0) {
	// This is a contingency involving a new strategy... skip
	newindex = -1L;
	break;
      }
      else {
	newindex += (iter.profile.GetStrategy(pl)->m_number - 1) * offsets[pl];
      }
    }

    if (newindex >= 0) {
      newResults[newindex+1] = results[iter.profile.m_index+1];
    }
  } while (iter.NextContingency());

  results = newResults;

  IndexStrategies();
}

