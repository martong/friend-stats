//============================================================================
//befriending class: gdcm::Sorter
//friendly function: gdcm::operator<<
//friendDeclLoc: /Users/mg/WORK/friend/measure/ITK/src/ITK/Modules/ThirdParty/GDCM/src/gdcm/Source/MediaStorageAndFileFormat/gdcmSorter.h:41:24
//defLoc: /Users/mg/WORK/friend/measure/ITK/src/ITK/Modules/ThirdParty/GDCM/src/gdcm/Source/MediaStorageAndFileFormat/gdcmSorter.h:72:22
//diagName: gdcm::operator<<
//usedPrivateVarsCount: 0
//parentPrivateVarsCount: 3
//usedPrivateMethodsCount: 0
//parentPrivateMethodsCount: 0
//types.usedPrivateCount: 0
//types.parentPrivateCount: 1
//============================================================================

// Befreinding class defintion:
class GDCM_EXPORT Sorter
{
  friend std::ostream& operator<<(std::ostream &_os, const Sorter &s);
public:
  // ...
  void Print(std::ostream &os) const;
protected:
  std::vector<std::string> Filenames;
  // ...
};


// Friend function definition:
inline std::ostream& operator<<(std::ostream &os, const Sorter &s)
{
  s.Print( os );
  return os;
}

// op<< uses the public Print function only.
