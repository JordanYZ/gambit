//#
//# FILE: enum.cc -- Enum module
//#
//# $Id$
//#

#include "rational.h"
#include "gwatch.h"
#include "gpvector.h"

#include "normal.h"
#include "normiter.h"

#include "enum.h"

template <class T> gMatrix<T> Make_A(const NormalForm<T> &);
template <class T> gVector<T> Make_b(const NormalForm<T> &);

//---------------------------------------------------------------------------
//                        EnumParams: member functions
//---------------------------------------------------------------------------

EnumParams::EnumParams(gStatus &status_) : plev(0), stopAfter(0),
outfile(&gnull),status(status_)
{ }

//-------------------------------------------------------------------------
//                    EnumModule<T>: Member functions
//-------------------------------------------------------------------------

template <class T>
EnumModule<T>::EnumModule(const NormalForm<T> &N, const EnumParams &p)
  : NF(N), params(p), rows(N.NumStrats(1)), cols(N.NumStrats(2)), 
    level(0), count(0), npivots(0)
{ }


template <class T> int EnumModule<T>::Enum(void)
{
  // Ted -- is there a better way to do this?  A lot of 
  //        allocation before finding out there are too 
  //        many players. (Same in Lemke module I think)
  
  if (NF.NumPlayers() != 2)   return 0;  
  
  gWatch watch;
  
  gBlock<int> target(rows+cols);
  for(int i=1;i<=target.Length();i++)
    target[i]=i;
  
  gMatrix<T> A(Make_A(NF));
  gVector<T> b(Make_b(NF));
  LTableau<T> tableau(A,b);
//  gout << "\n in Enum()";
//  tableau.Dump(gout);
  
  for(i=rows+1; i<=rows+cols && !params.status.Get();i++ ) {
    if(params.stopAfter==0 || List.Length()<params.stopAfter) 
      SubSolve(rows,i,tableau,target);
    params.status.SetProgress((double)(i-rows-1)/(double)cols);
  }
  
  if(params.status.Get()) {
    (*params.outfile) << "\n User Break \n";
    params.status.Reset();
  }

  if(params.plev>=2) {
    for(i=1;i<=List.Length();i++) {
      (*params.outfile) << "\n";
      List[i].Dump(*params.outfile);
    }
  }
  time = (gRational) watch.Elapsed();
  return 1;
}


template <class T> void EnumModule<T>
::SubSolve(int pr, int pcl, LTableau<T> &B1, gBlock<int> &targ1)
{
  int i,j,ii,jj,pc;
  count++;
  LTableau<T> B2(B1);
  B2.NumPivots()=0;

  // construct new target basis
  gBlock<int> targ2(targ1);  
  pc = targ1.Find(pcl);
  targ2[pc] = targ2[pr];
  targ2[pr] = pcl;
  
//  gout << "\n targ = ";
//  targ2.Dump(gout);
  
  
  /* pivot to target */
  int flag = 1;
  int piv = 1;
  while(piv && flag) {
    piv=0;
    flag=0;
    for(i=1;i<=rows;i++) {
      ii = targ2[i];
      if(ii<=rows)ii=-ii;
      if(!B2.Member(ii)) {
//	gout << " i,ii : " << i << ii;
	j=rows+1;
	jj = targ2[j];
	if(jj<=rows)jj=-jj;
	while(j<=rows+cols && !B2.Member(jj)) {
	  j++;
	  jj = targ2[j];
	  if(jj<=rows)jj=-jj;
	}
//	gout << " j,jj : " << j << jj;
	if(j<=rows+cols) {
	  // note: may want to pivot for 1 and 2 separately to pick 
	  // up additional possible feasible solutions.  
	  if(B2.CanPivot(jj,ii) && B2.CanPivot(-ii,-jj))  {
//	    gout << " jj,ii, : " << i << j << ii << jj;
	    B2.CompPivot(jj,ii);
	    piv=1;
	  }
	  else flag=1;
	}
      }
    }
  }
//  gout << "\n";
//  B2.Dump(gout);
  
  npivots+=B2.NumPivots();
  j=0;
  if(B2.IsNash()) {
    List.Append(B2.GetBFS());
    j=1;
  }
  
  if(params.plev>=3) {
    printf("\nPass# %3ld, Depth =%3d, Target = ",
	   count, rows-pr+1);
    for(i=1;i<=rows;i++)
      printf("%3d", targ2[i]);
    if(flag) {
      printf("  Infeasible");
//      B2.Dump(gout); 
    }
    if(j) {
      printf("  Nash equilib");    
      B2.Dump(*params.outfile); 
    }
  }
  
  if(flag) B2=B1;
  
  if(pr>1) {
    for(i=targ2[pr-1]+1;i<targ2[pr] && !params.status.Get();i++)
      if(params.stopAfter==0 || List.Length()<params.stopAfter) {
	SubSolve(pr-1,i,B2,targ2);
      }
  }
}

/*
template <class T>
gList<gPVector<T> > &EnumTableau<T>::AddSolution(void) const
{
  gArray<int> dim(2);
  dim[1] = n1;
  dim[2] = n2;
  
  gPVector<T> profile(dim);
  T sum = (T) 0;
  
  for (int j = 1; j <= N.NumStrats(1); j++)
    if (List.IsDefined(j))   sum += List(j);
  
  if (sum == (T) 0)  continue;
  
  for (j = 1; j <= N.NumStrats(1); j++) 
    if (List.IsDefined(j))   profile(1, j) = List(j) / sum;
    else  profile(1, j) = (T) 0;
  
  sum = (T) 0;
  
  for (j = 1; j <= N.NumStrats(2); j++)
    if (List[i].IsDefined(N.NumStrats(1) + j))  
      sum += List[i](N.NumStrats(1) + j);
  
  if (sum == (T) 0)  continue;
  
  for (j = 1; j <= N.NumStrats(2); j++)
    if (List[i].IsDefined(N.NumStrats(1) + j))
      profile(2, j) = List[i](N.NumStrats(1) + j) / sum;
    else
      profile(2, j) = (T) 0;
  
  solutions.Append(profile);
}
*/

template <class T> long EnumModule<T>::NumPivots(void) const
{
  return npivots;
}

template <class T> double EnumModule<T>::Time(void) const
{
  return time;
}

template <class T> EnumParams &EnumModule<T>::Parameters(void)
{
  return params;
}

//template <class T> void EnumModule<T>
//::GetSolutions(gList<gPVector<T> > &solutions) const

template <class T> gList<gPVector<T> > &EnumModule<T>
::GetSolutions(void) const
{
  gList<gPVector<T> > *solutions;
  solutions = new gList<gPVector<T> >;
//  solutions.Flush();

  for (int i = 1; i <= List.Length(); i++)    {
    gArray<int> dim(2);
    dim[1] = rows;
    dim[2] = cols;

    gPVector<T> profile(dim);
    T sum = (T) 0;

    for (int j = 1; j <= rows; j++)
      if (List[i].IsDefined(j))   sum += List[i](j);

    if (sum == (T) 0)  continue;

    for (j = 1; j <= rows; j++) 
      if (List[i].IsDefined(j))   profile(1, j) = List[i](j) / sum;
      else  profile(1, j) = (T) 0;

    sum = (T) 0;

    for (j = 1; j <= cols; j++)
      if (List[i].IsDefined(rows + j))  
	sum += List[i](rows + j);

    if (sum == (T) 0)  continue;

    for (j = 1; j <= cols; j++)
      if (List[i].IsDefined(rows + j))
	profile(2, j) = List[i](rows + j) / sum;
      else
	profile(2, j) = (T) 0;

    solutions->Append(profile);
  }
  return *solutions;
}


/*
template <class T>
const gList<gPVector<T> > &EnumModule<T>::GetSolutions(void) const
{
  return solutions;
}


template <class T>
void EnumModule<T>::AddSolution(const gPVector<T> &s)
{
  solutions.Append(s);
}
*/
#ifdef __GNUG__
template class EnumModule<double>;
template class EnumModule<gRational>;
#elif defined __BORLANDC__
#pragma option -Jgd
class EnumModule<double>;
class EnumModule<gRational>;
#pragma option -Jgx
#endif   // __GNUG__, __BORLANDC__


//-------------------------------------------------------------------------
//                    Convenience functions for Enum
//-------------------------------------------------------------------------

template <class T>
int Enum(const NormalForm<T> &N, const EnumParams &p,
	  gList<gPVector<T> > &solutions,
	  long &npivots, gRational &time)
{
  EnumModule<T> LM(N, p);
  int result = LM.Enum();

  npivots = LM.NumPivots();
  time = LM.Time();
  
  LM.GetSolutions();

  return result;
}

#ifdef __GNUG__
template int Enum(const NormalForm<double> &, const EnumParams &,
		   gList<gPVector<double> > &, int &, gRational &);
template int Enum(const NormalForm<gRational> &, const EnumParams &,
		   gList<gPVector<gRational> > &, int &, gRational &);
#elif defined __BORLANDC__
#pragma option -Jgd
int Enum(const NormalForm<double> &, const EnumParams &,
	  gList<gPVector<double> > &, int &, gRational &);
int Enum(const NormalForm<gRational> &, const EnumParams &,
	  gList<gPVector<gRational> > &, int &, gRational &);
#pragma option -Jgx
#endif   // __GNUG__, __BORLANDC__










