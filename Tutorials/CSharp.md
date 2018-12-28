### What are the keywords in C#? ###
---
[Link: List of keywords and details](https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/keywords/)
* Modifiers 
* Access Modifiers
  * public
  * private
  * internal
  * protected
* Statement
  * Selection (if-else, switch)
  * Iteration (do, for, foreach, in, while)
  * Jump (break, continue, goto, return)
  * Exception Handling (throw, try, catch, finaly)
  * checked and unchecked
* Method Parameter (params, in, ref, out)
  * *Parameters declared for a method without in, ref or out, are passed to the called method by value.*
* Operator (as, await, is, new, sizeof, typeof, true, false, stackalloc, nameof)
* Namespace (namespace, using, using static, .Operator, ::Operator, extern alias)
* Access 
* Literal 
* Type 
* Query 
* Contextual

### Define property? ###
---
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

### What is the use of Checked and Unchecked keywords? ###
---
Checked: Adds exceptions on number overflows.
Unchecked: Numeric types cause no exceptions when they overflow. This is the default behavior.
```csharp
while (true) {
    checked{
        a++; // Will throw overflow error
    }
    unchecked{
        b++;
    }
}
```

#### If default behaviour is Unchecked, then why there is one? ####
The unchecked keyword is used to suppress overflow-checking for integral-type arithmetic operations and conversions. The overflow can be detected at compile time because all the terms of the expression are constants. For example, to supress the overflow checking when adding two const variables.
```csharp
unchecked
{
    int1 = 2147483647 + 10;
}
```

### What is `using static` namespace keyword? ###
The using static directive designates a type whose static members and nested types you can access without specifying a type name. 
```csharp
public double Area
{
  get { return Math.PI * Math.Pow(Radius, 2); }
}
```
can be used as; to have cleaner code;
```csharp
using static System.Math;
...
public double Area
{
  get { return PI * Pow(Radius, 2); }
}
```   