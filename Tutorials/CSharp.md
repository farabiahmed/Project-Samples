### What are the keywords in C#? ###
[Link: List of keywords and details](https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/keywords/modifiers)
* Modifiers 
* Access Modifiers
  * public
  * private
  * internal
  * protected
* Statement
* Method Parameter: *Parameters declared for a method without in, ref or out, are passed to the called method by value.*
  * params
  * in
  * ref
  * out
* Operator
* Namespace 
* Access 
* Literal 
* Type 
* Query 
* Contextual

### Define property? ###
By the property we can access private fields.
We have the three types of properties:
* Read/Write.
* ReadOnly.
* WriteOnly

Example for read/write:
```csharp
public string Name  {
 get{ return mName;}
 set{ mName = value;}        
}
```
