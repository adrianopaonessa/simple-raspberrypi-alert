# Define parameters for the toast notification
param (
    [string]$Title = "Raspberry Pi Alert.",
    [string]$Message = "The button has been pressed!"
)

# Load Windows Runtime types for toast notifications
[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
[Windows.UI.Notifications.ToastNotification, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
[Windows.UI.Notifications.ToastTemplateType, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
[Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType = WindowsRuntime] | Out-Null

# Select the toast notification template with two text fields
$template = [Windows.UI.Notifications.ToastTemplateType]::ToastText02

# Get the XML content for the selected template
$xml = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent($template)

# Set the first text field (title) in the XML to the provided $Title parameter
$xml.SelectSingleNode("//text[@id='1']").InnerText = $Title

# Set the second text field (message) in the XML to the provided $Message parameter
$xml.SelectSingleNode("//text[@id='2']").InnerText = $Message

# Create a new ToastNotification object using the modified XML
$toast = [Windows.UI.Notifications.ToastNotification]::new($xml)

# Display the toast notification using a custom notifier name
[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier("Raspberry Pi Notifier").Show($toast)