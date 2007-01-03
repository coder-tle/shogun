%{
 #include "features/StringFeatures.h" 
%}

%apply (ST** ARGOUT1, INT* DIM1) {(ST** dst, INT* len)};
%apply (CHAR** ARGOUT1, INT* DIM1) {(CHAR** dst, INT* len)};
%apply (BYTE** ARGOUT1, INT* DIM1) {(BYTE** dst, INT* len)};
%apply (SHORT** ARGOUT1, INT* DIM1) {(SHORT** dst, INT* len)};
%apply (WORD** ARGOUT1, INT* DIM1) {(WORD** dst, INT* len)};
%apply (INT** ARGOUT1, INT* DIM1) {(INT** dst, INT* len)};
%apply (UINT** ARGOUT1, INT* DIM1) {(UINT** dst, INT* len)};
%apply (LONG** ARGOUT1, INT* DIM1) {(LONG** dst, INT* len)};
%apply (SHORTREAL** ARGOUT1, INT* DIM1) {(SHORTREAL** dst, INT* len)};
%apply (DREAL** ARGOUT1, INT* DIM1) {(DREAL** dst, INT* len)};
%apply (LONGREAL** ARGOUT1, INT* DIM1) {(LONGREAL** dst, INT* len)};

%include "features/StringFeatures.h" 

%template(StringCharFeatures) CStringFeatures<CHAR>;
%template(StringByteFeatures) CStringFeatures<BYTE>;
%template(StringWordFeatures) CStringFeatures<WORD>;
%template(StringUlongFeatures) CStringFeatures<ULONG>;
